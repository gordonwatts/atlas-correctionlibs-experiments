/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// JetVertexFractionTool.cxx

#include "JetMomentTools/JetVertexFractionTool.h"
#include "AsgDataHandles/ReadDecorHandle.h"
#include "AsgDataHandles/WriteDecorHandle.h"

//**********************************************************************

JetVertexFractionTool::JetVertexFractionTool(const std::string& name)
  : asg::AsgTool(name) {
}

//**********************************************************************

StatusCode JetVertexFractionTool::initialize() {
  ATH_MSG_DEBUG("Initializing JetVertexFractionTool " << name());
  ATH_MSG_DEBUG("Using origin vertex: " << m_useOriginVertex);

  if(m_jetContainerName.empty()){
    ATH_MSG_ERROR("JetVertexFractionTool needs to have its input jet container name configured!");
    return StatusCode::FAILURE;
  }

  if ( m_htsel.empty() ) {
    ATH_MSG_DEBUG("  No track selector.");
  } else {
    ATH_MSG_DEBUG("  Track selector: " << m_htsel->name());
  }
  ATH_MSG_DEBUG("  Attribute name: " << m_jvfKey.key());

  m_sumPtTrkKey = m_jetContainerName + "." + m_sumPtTrkKey.key();
  m_jvfKey = m_jetContainerName + "." + m_jvfKey.key();
  m_jvfCorrKey = m_jetContainerName + "." + m_jvfCorrKey.key();
  m_maxJvfVtxKey = m_jetContainerName + "." + m_maxJvfVtxKey.key();
  m_jvfCorrVtxHandleKey = m_jvfCorrKey.key() + "Vec";

#ifndef XAOD_STANDALONE
  if(m_suppressInputDeps){
    // The user has promised that this will be produced by the same alg running JVF.
    // Tell the scheduler to ignore it to avoid circular dependencies.
    renounce(m_sumPtTrkKey);
  }
#endif

  ATH_CHECK(m_vertexContainer_key.initialize());
  ATH_CHECK(m_tva_key.initialize());
  ATH_CHECK(m_tracksCont_key.initialize());
  ATH_CHECK(m_sumPtTrkKey.initialize());
  ATH_CHECK(m_jvfKey.initialize());
  ATH_CHECK(m_jvfCorrKey.initialize());
  ATH_CHECK(m_maxJvfVtxKey.initialize(!m_isTrigger));
  ATH_CHECK(m_jvfCorrVtxHandleKey.initialize(m_useOriginVertex));

  return StatusCode::SUCCESS;
}

StatusCode JetVertexFractionTool::decorate(const xAOD::JetContainer& jetCont) const {

  // Get the vertices container
  auto vertexContainer = SG::makeHandle (m_vertexContainer_key);
  if (!vertexContainer.isValid()){
    ATH_MSG_WARNING("Invalid  xAOD::VertexContainer datahandle"
                    << m_vertexContainer_key.key()); 
    return StatusCode::FAILURE;
  }
  const auto *vertices = vertexContainer.cptr();

  ATH_MSG_DEBUG("Successfully retrieved VertexContainer: " 
                << m_vertexContainer_key.key()); 

  // Get the Tracks container
  auto tracksContainer = SG::makeHandle (m_tracksCont_key);
  if (!tracksContainer.isValid()){
    ATH_MSG_ERROR("Could not retrieve the TrackParticleContainer: " 
                  << m_tracksCont_key.key());
    return StatusCode::FAILURE;
  }
  const auto *tracksCont = tracksContainer.cptr();

  ATH_MSG_DEBUG("Successfully retrieved TrackParticleContainer: " 
                << m_tracksCont_key.key());

  // Get the TVA object
  auto tvaContainer = SG::makeHandle (m_tva_key);
  if (!tvaContainer.isValid()){
    ATH_MSG_ERROR("Could not retrieve the TrackVertexAssociation: " 
                  << m_tva_key.key());
    return StatusCode::FAILURE;
  }
  const auto *tva = tvaContainer.cptr();

  ATH_MSG_DEBUG("Successfully retrieved TrackVertexAssociation: " 
                << m_tva_key.key());

  if (vertices->empty() ) {
    ATH_MSG_WARNING("There are no vertices in the container. Exiting");
    return StatusCode::SUCCESS;
  }

  // Get the vertex to calculate with respect to
  // Only appropriate if we are using a single vertex interpretation, not if using OriginVertex
  const xAOD::Vertex* HSvertex = nullptr;
  if (!m_useOriginVertex)
    HSvertex = findHSVertex(vertices);

  // Count pileup tracks - currently done for each collection
  // Only appropriate if we are using a single vertex interpretation, not if using OriginVertex
  const int n_putracks = !m_useOriginVertex ? getPileupTrackCount(HSvertex, tracksCont, tva) : -1;

  SG::ReadDecorHandle<xAOD::JetContainer, std::vector<float> > sumPtTrkHandle(m_sumPtTrkKey);
  SG::WriteDecorHandle<xAOD::JetContainer, std::vector<float> > jvfHandle(m_jvfKey);
  SG::WriteDecorHandle<xAOD::JetContainer, float> jvfCorrHandle(m_jvfCorrKey);

  // We don't want to initialize this handle if this is trigger
  std::unique_ptr<SG::WriteDecorHandle<xAOD::JetContainer, ElementLink<xAOD::VertexContainer> > > maxJvfVtxHandle;
  if(!m_isTrigger)
    maxJvfVtxHandle = std::make_unique<SG::WriteDecorHandle<xAOD::JetContainer, ElementLink<xAOD::VertexContainer> > >(m_maxJvfVtxKey);

  for(const xAOD::Jet * jet : jetCont) {
    // Get origin-vertex-specific information if relevant
    if (m_useOriginVertex)
    {
      HSvertex = jet->getAssociatedObject<xAOD::Vertex>("OriginVertex");
      if (!HSvertex) // nullptr if the attribute doesn't exist
      {
        ATH_MSG_ERROR("OriginVertex was requested, but the jet does not contain an OriginVertex");
        return StatusCode::FAILURE;
      }
      else
        ATH_MSG_VERBOSE("JetVertexFractionTool " << name() << " is using OriginVertex at index: " << HSvertex->index());
    }
    const int n_putracks_local = !m_useOriginVertex ? n_putracks : getPileupTrackCount(HSvertex,tracksCont,tva);

    // Get the tracks associated to the jet
    // Note that there may be no tracks - this is both normal and an error case
    std::vector<const xAOD::TrackParticle*> tracks;
    if ( ! jet->getAssociatedObjects(m_assocTracksName, tracks) ) {
      ATH_MSG_DEBUG("Associated tracks not found.");
    }

    // Get the track pT sums for all tracks in the jet (first key) and those associated to PU (second key) vertices.
    const std::pair<float,float> tracksums = getJetVertexTrackSums(HSvertex, tracks, tva);
    // Get the track pT sums for each individual vertex
    std::vector<float> vsumpttrk = sumPtTrkHandle(*jet);
    float sumpttrk_all = tracksums.first;
    float sumpttrk_nonPV = tracksums.second;
    float sumpttrk_PV = vsumpttrk[HSvertex->index() - (*vertices)[0]->index()];

    // Get and set the JVF vector
    std::vector<float> jvf(vertices->size());
    for(size_t vtxi=0; vtxi<vertices->size(); ++vtxi) {
      jvf[vtxi] = sumpttrk_all > 1e-9 ? vsumpttrk[vtxi] / sumpttrk_all : -1;
    }
    jvfHandle(*jet) = jvf;

    // Get and set the highest JVF vertex
    if(!m_isTrigger) {
      (*maxJvfVtxHandle)(*jet) = getMaxJetVertexFraction(vertices,jvf);
    }
    // Calculate JVFCorr
    // Default JVFcorr to -1 when no tracks are associated.
    float jvfcorr = -999.;
    if(sumpttrk_PV + sumpttrk_nonPV > 0) {
      jvfcorr = sumpttrk_PV / (sumpttrk_PV + ( sumpttrk_nonPV / (m_kcorrJVF * std::max(n_putracks_local, 1) ) ) );
    } else {
      jvfcorr = -1;
    }
    jvfCorrHandle(*jet) = jvfcorr;
  }

  if (m_useOriginVertex) { // Add extra info to compute JVT for jets assuming other vertices as origin
    std::vector<float> jvfCorrVtx;
     
    auto jvfCorrVtxHandle = std::make_unique<SG::WriteDecorHandle<xAOD::JetContainer, std::vector<float> > >(m_jvfCorrVtxHandleKey);

    for(const xAOD::Jet * jet : jetCont) {      
      jvfCorrVtx.clear();
      std::vector<float> vsumpttrk = sumPtTrkHandle(*jet);

      // Loop over vertices
      for(const xAOD::Vertex* pv : *vertices){

        // Calculate JVFCorr for a given vertex
        // Default JVFcorr to -1 when no tracks are associated. -  copied from JetVertexFractionTool.cxx
        // Get the tracks associated to the jet
        // Note that there may be no tracks - this is both normal and an error case
        std::vector<const xAOD::TrackParticle*> tracks;
        if ( ! jet->getAssociatedObjects(m_assocTracksName, tracks) ) {
          ATH_MSG_DEBUG("Associated tracks not found.");
        }
      
        const int n_putracks = getPileupTrackCount(pv, tracksCont, tva);

        // Get the track pT sums for all tracks in the jet (first key) and those associated to PU(?) (second key) vertices.
        const std::pair<float,float> tracksums = getJetVertexTrackSums(pv, tracks, tva);
        // Get the track pT sums for each individual vertex
    
        float sumpttrk_PV = vsumpttrk[pv->index()];
        float sumpttrk_nonPV = tracksums.second; // Consider as "PU" all vertices not matching the one I'm looping over
        float jvfcorr = -999.;
        float kcorrJVF = 0.01;
        if(sumpttrk_PV + sumpttrk_nonPV > 0) { 
          jvfcorr = sumpttrk_PV / (sumpttrk_PV + ( sumpttrk_nonPV / (kcorrJVF * std::max(n_putracks, 1) ) ) );
        } else {
          jvfcorr = -1;
        }
        jvfCorrVtx.push_back(jvfcorr);
      }

      (*jvfCorrVtxHandle)(*jet) = jvfCorrVtx;
      // Done
      
    } 
  }
  return StatusCode::SUCCESS;
}

//**********************************************************************

ElementLink<xAOD::VertexContainer> JetVertexFractionTool::
getMaxJetVertexFraction(const xAOD::VertexContainer* vertices,
                        const std::vector<float>& jvf) const {
  size_t maxIndex = 0;
  float maxVal = -100;
  for ( size_t iVertex = 0; iVertex < jvf.size(); ++iVertex ) {
    if ( jvf.at(iVertex) > maxVal ) {
      maxIndex = iVertex;
      maxVal = jvf.at(iVertex);
    }
  }
  ElementLink<xAOD::VertexContainer> link =
    ElementLink<xAOD::VertexContainer>(*vertices,vertices->at(maxIndex)->index());
  return link;
}

//**********************************************************************

std::pair<float,float> JetVertexFractionTool::getJetVertexTrackSums(const xAOD::Vertex* vertex,
                                                                  const std::vector<const xAOD::TrackParticle*>& tracks,
                                                                  const jet::TrackVertexAssociation* tva) const {
    float sumTrackAll = 0;
    float sumTracknotPV = 0;
    bool notsel = m_htsel.empty();
    unsigned int nkeep = 0;
    unsigned int nskip = 0;
    for (size_t iTrack = 0; iTrack < tracks.size(); ++iTrack) {
      const xAOD::TrackParticle* track = tracks.at(iTrack);
      if ( notsel || m_htsel->accept(*track) ) {
        sumTrackAll += track->pt();

        const xAOD::Vertex* ptvtx = tva->associatedVertex(track);
        if( ptvtx != nullptr ) {
          // Track has vertex, assign to appropriate pT sum
          if ( ptvtx->index() != vertex->index() ) {sumTracknotPV += track->pt(); }
        }
        ++nkeep;
      }
      else { ++nskip; }
    }
    ATH_MSG_VERBOSE("JetVertexTaggerTool " << name()
		    << ": nsel=" << nkeep
		    << ", nrej=" << nskip           );

    return std::make_pair(sumTrackAll,sumTracknotPV);

}


//**********************************************************************

int JetVertexFractionTool::getPileupTrackCount(const xAOD::Vertex* vertex,
					     const xAOD::TrackParticleContainer*& tracksCont,
					     const jet::TrackVertexAssociation* tva) const
{
    int n_pileuptrackcount = 0;
    bool notsel = m_htsel.empty();
    unsigned int nkeep = 0;
    unsigned int nskip = 0;
    int tot_count = 0;
    for(size_t iTrack = 0; iTrack < tracksCont->size(); ++iTrack)
      {
	const xAOD::TrackParticle * track = tracksCont->at(iTrack);
	if ( notsel || m_htsel->accept(*track) ) {
	  const xAOD::Vertex* ptvtx = tva->associatedVertex(track);
	  // Count track as PU if associated with non-primary vertex and within pT cut.
	  // N.B. tracks with no vertex associated may be added to PV track sums, but not PU sums, nor the PU vertex counting.
	  if ( ptvtx != nullptr ) {
	    if ( (ptvtx->index() != vertex->index() ) && (track->pt() < m_PUtrkptcut) ) ++n_pileuptrackcount;
	  }
	  tot_count++;
	  ++nkeep;
	}
	else { ++nskip; }
      }
    const int n_pileuptracks = n_pileuptrackcount;

    ATH_MSG_VERBOSE("JetVertexFractionTool " << name()
                  << ": nsel=" << nkeep
                  << ", nrej=" << nskip
		 << ", total " << tracksCont->size() );
    ATH_MSG_VERBOSE("JetVertexFractionTool " << name()
		 << ": n_PUtracks=" << n_pileuptracks
		 << ", total=" << tot_count          );

    return n_pileuptracks;
}

//**********************************************************************

const xAOD::Vertex* JetVertexFractionTool::findHSVertex(const xAOD::VertexContainer*& vertices) const
{
  const xAOD::Vertex* primvert = nullptr;
  for (const xAOD::Vertex* pv : *vertices) {
	if (pv->vertexType() == xAOD::VxType::PriVtx ) {
		primvert = pv;
      		ATH_MSG_VERBOSE("JetVertexFractionTool " << name() << " Found HS vertex.");
		break;
	}
  }
  if (primvert == nullptr ) {
  	ATH_MSG_VERBOSE("There is no vertex of type PriVx. Taking default vertex.");
	primvert = *(vertices->begin());
  }
  return primvert;
}
