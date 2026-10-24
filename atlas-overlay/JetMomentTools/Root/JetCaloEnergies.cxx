/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "JetMomentTools/JetCaloEnergies.h"

#include "xAODJet/JetAccessorMap.h"
#include "AsgDataHandles/WriteDecorHandle.h"

#include "JetUtils/JetCaloQualityUtils.h"
#include "PFlowUtils/FEHelpers.h"

#include "xAODCaloEvent/CaloCluster.h"
#include "xAODPFlow/PFO.h"
#include "xAODPFlow/FlowElement.h"

#include <memory>
#include <vector>

//**********************************************************************

JetCaloEnergies::JetCaloEnergies(const std::string& name)
: AsgTool(name) { }

//**********************************************************************

bool JetCaloEnergies::isInVector(const std::string& key, const std::vector<std::string>& calculations) {
  // Split key to get back the actual handle key 
  std::vector<std::string> split;
  std::string sub_string;
  std::istringstream tokenStream(key);

  while (std::getline(tokenStream, sub_string, '.'))
  {
    split.push_back(sub_string);
  }

  // Return true if handle key in list of calculations
  return std::find(calculations.begin(), calculations.end(), split[1]) != calculations.end();
}

StatusCode JetCaloEnergies::initialize() {
  ATH_MSG_DEBUG("Initializing JetCaloEnergies " << name());
  
  if(m_jetContainerName.empty()){
    ATH_MSG_ERROR("JetCaloEnergies needs to have its input jet container configured!");
    return StatusCode::FAILURE;
  }

  m_ePerSamplingKey = m_jetContainerName + "." + m_ePerSamplingKey.key();
  m_emFracKey = m_jetContainerName + "." + m_emFracKey.key();
  m_hecFracKey = m_jetContainerName + "." + m_hecFracKey.key();
  m_psFracKey = m_jetContainerName + "." + m_psFracKey.key();
  m_em3FracKey = m_jetContainerName + "." + m_em3FracKey.key();
  m_tile0FracKey = m_jetContainerName + "." + m_tile0FracKey.key();
  m_effNClustsFracKey = m_jetContainerName + "." + m_effNClustsFracKey.key();
  m_fracSamplingMaxKey = m_jetContainerName + "." + m_fracSamplingMaxKey.key();
  m_fracSamplingMaxIndexKey = m_jetContainerName + "." + m_fracSamplingMaxIndexKey.key();

  if(m_calcClusterBasedVars){
    m_ePerSamplingClusterKey = m_jetContainerName + "." + m_ePerSamplingClusterKey.key();
    m_emFracClusterKey = m_jetContainerName + "." + m_emFracClusterKey.key();
    m_hecFracClusterKey = m_jetContainerName + "." + m_hecFracClusterKey.key();
    m_psFracClusterKey = m_jetContainerName + "." + m_psFracClusterKey.key();
    m_em3FracClusterKey = m_jetContainerName + "." + m_em3FracClusterKey.key();
    m_tile0FracClusterKey = m_jetContainerName + "." + m_tile0FracClusterKey.key();
    m_effNClustsFracClusterKey = m_jetContainerName + "." + m_effNClustsFracClusterKey.key();
    m_fracSamplingMaxClusterKey = m_jetContainerName + "." + m_fracSamplingMaxClusterKey.key();
    m_fracSamplingMaxIndexClusterKey = m_jetContainerName + "." + m_fracSamplingMaxIndexClusterKey.key();
    m_lambdaLeadingClusterKey = m_jetContainerName + "." + m_lambdaLeadingClusterKey.key();
    m_meanRadialDistanceSquaredKey = m_jetContainerName + "." + m_meanRadialDistanceSquaredKey.key();
    m_meanLongitudinalDistanceSquaredKey = m_jetContainerName + "." + m_meanLongitudinalDistanceSquaredKey.key();
  }

  // Init calo based variables if necessary
  ATH_CHECK(m_ePerSamplingClusterKey.initialize( m_calcClusterBasedVars ));
  ATH_CHECK(m_emFracClusterKey.initialize( m_calcClusterBasedVars && isInVector(m_emFracKey.key(), m_calculationNames) ));
  ATH_CHECK(m_hecFracClusterKey.initialize( m_calcClusterBasedVars && isInVector(m_hecFracKey.key(), m_calculationNames) ));
  ATH_CHECK(m_psFracClusterKey.initialize( m_calcClusterBasedVars && isInVector(m_psFracKey.key(), m_calculationNames) ));
  ATH_CHECK(m_em3FracClusterKey.initialize( m_calcClusterBasedVars && isInVector(m_em3FracKey.key(), m_calculationNames) ));
  ATH_CHECK(m_tile0FracClusterKey.initialize( m_calcClusterBasedVars && isInVector(m_tile0FracKey.key(), m_calculationNames) ));
  ATH_CHECK(m_effNClustsFracClusterKey.initialize( m_calcClusterBasedVars && isInVector(m_effNClustsFracKey.key(), m_calculationNames) ));
  
  m_doFracSamplingMax = isInVector(m_fracSamplingMaxKey.key(), m_calculationNames);

  ATH_CHECK(m_fracSamplingMaxClusterKey.initialize(m_calcClusterBasedVars && m_doFracSamplingMax));
  ATH_CHECK(m_fracSamplingMaxIndexClusterKey.initialize(m_calcClusterBasedVars && m_doFracSamplingMax));

  // Init standard variables if necessary
  ATH_CHECK(m_ePerSamplingKey.initialize());
  ATH_CHECK(m_emFracKey.initialize());
  ATH_CHECK(m_hecFracKey.initialize());
  ATH_CHECK(m_psFracKey.initialize());
  ATH_CHECK(m_em3FracKey.initialize( isInVector(m_em3FracKey.key(), m_calculationNames) ));
  ATH_CHECK(m_tile0FracKey.initialize( isInVector(m_tile0FracKey.key(), m_calculationNames) ));
  ATH_CHECK(m_effNClustsFracKey.initialize( isInVector(m_effNClustsFracKey.key(), m_calculationNames) ));
  ATH_CHECK(m_fracSamplingMaxKey.initialize(m_doFracSamplingMax));
  ATH_CHECK(m_fracSamplingMaxIndexKey.initialize(m_doFracSamplingMax));
  ATH_CHECK(m_lambdaLeadingClusterKey.initialize());
  ATH_CHECK(m_meanRadialDistanceSquaredKey.initialize());
  ATH_CHECK(m_meanLongitudinalDistanceSquaredKey.initialize());
  
  return StatusCode::SUCCESS;
}

//**********************************************************************

StatusCode JetCaloEnergies::decorate(const xAOD::JetContainer& jets) const {
  ATH_MSG_VERBOSE("Begin decorating jets.");
  for(const xAOD::Jet* jet : jets) {
    SG::WriteDecorHandle<xAOD::JetContainer, std::vector<float> > ePerSamplingHandle(m_ePerSamplingKey);
    size_t numConstit = jet->numConstituents();
    ePerSamplingHandle(*jet) = std::vector<float>(CaloSampling::Unknown, 0.);
    std::vector<float>& ePerSampling = ePerSamplingHandle(*jet);
    for ( float& e : ePerSampling ) e = 0.0; // re-initialize

    if ( numConstit == 0 ) {
      ATH_MSG_VERBOSE("Jet has no constituents.");
      continue;
    }
    
    // should find a more robust solution than using 1st constit type.
    xAOD::Type::ObjectType ctype = jet->rawConstituent( 0 )->type();
    if ( ctype  == xAOD::Type::CaloCluster ) {
      ATH_MSG_VERBOSE("  Constituents are calo clusters.");
      fillEperSamplingCluster(*jet, ePerSampling);

    } else if (ctype  == xAOD::Type::ParticleFlow) {
      ATH_MSG_VERBOSE("  Constituents are pflow objects.");
      fillEperSamplingPFO(*jet, ePerSampling);

    } else if (ctype  == xAOD::Type::FlowElement) {
      ATH_MSG_VERBOSE("  Constituents are FlowElements.");
      fillEperSamplingFE(*jet, ePerSampling);

      // In addition, calculate variables using the underlying cluster rather than
      // the energy-subtracted FlowElements (improved implementation)
      if(m_calcClusterBasedVars){
        ATH_MSG_VERBOSE("  Constituents are FlowElements - Additional calculation");

        SG::WriteDecorHandle<xAOD::JetContainer, std::vector<float> > ePerSamplingClusterHandle(m_ePerSamplingClusterKey);
        ePerSamplingClusterHandle(*jet) = std::vector<float>(CaloSampling::Unknown, 0.);
        std::vector<float>& ePerSamplingCluster = ePerSamplingClusterHandle(*jet);
        for ( float& e : ePerSamplingCluster ) e = 0.0; // re-initialize

        fillEperSamplingFEClusterBased(*jet, ePerSamplingCluster);
      }

    }else {
      ATH_MSG_VERBOSE("Constituents are not CaloClusters, PFOs, or FlowElements.");
    }

  }
  return StatusCode::SUCCESS;
}

void JetCaloEnergies::fillEperSamplingCluster(const xAOD::Jet& jet, std::vector<float> & ePerSampling ) const {
  // loop over raw constituents
  size_t numConstit = jet.numConstituents();    
  for ( size_t i=0; i<numConstit; i++ ) {
    if(jet.rawConstituent(i)->type()!=xAOD::Type::CaloCluster) {
      ATH_MSG_WARNING("Tried to call fillEperSamplingCluster with a jet constituent that is not a cluster!");
      continue;
    }
    const xAOD::CaloCluster* constit = static_cast<const xAOD::CaloCluster*>(jet.rawConstituent(i));      
    for ( size_t s= CaloSampling::PreSamplerB; s< CaloSampling::Unknown; s++ ) {
      ePerSampling[s] += constit->eSample( (xAOD::CaloCluster::CaloSample) s );
    }
  }

  double fracSamplingMax = -999999999.;
  int fracSamplingMaxIndex = -1;
  double sumE_samplings = 0.0;

  if(m_doFracSamplingMax){
    for(unsigned int i = 0; i < ePerSampling.size(); ++i){
      double e = ePerSampling[i];
      sumE_samplings += e;
      if (e>fracSamplingMax){
        fracSamplingMax=e;
        fracSamplingMaxIndex = i;
      }
    }
  }

  SG::WriteDecorHandle<xAOD::JetContainer, float> emFracHandle(m_emFracKey);
  SG::WriteDecorHandle<xAOD::JetContainer, float> hecFracHandle(m_hecFracKey);
  SG::WriteDecorHandle<xAOD::JetContainer, float> psFracHandle(m_psFracKey);
  
  emFracHandle(jet) = jet::JetCaloQualityUtils::emFraction( ePerSampling );
  hecFracHandle(jet) = jet::JetCaloQualityUtils::hecF( &jet );
  psFracHandle(jet) = jet::JetCaloQualityUtils::presamplerFraction( &jet );

  if(m_doFracSamplingMax){
    SG::WriteDecorHandle<xAOD::JetContainer, float> fracSamplingMaxHandle(m_fracSamplingMaxKey);
    fracSamplingMaxHandle(jet) = sumE_samplings != 0. ? fracSamplingMax/sumE_samplings : 0.;
    SG::WriteDecorHandle<xAOD::JetContainer, int> fracSamplingMaxIndexHandle(m_fracSamplingMaxIndexKey);
    fracSamplingMaxIndexHandle(jet) = fracSamplingMaxIndex;
  }
}

#define FillESamplingPFO( LAYERNAME )                                        \
  float E_##LAYERNAME = 0.0;                                                \
  if (constit->attribute(xAOD::PFODetails::eflowRec_LAYERENERGY_##LAYERNAME, E_##LAYERNAME)) { \
    ePerSampling[CaloSampling::LAYERNAME] += E_##LAYERNAME;                \
  }
  
void JetCaloEnergies::fillEperSamplingPFO(const xAOD::Jet & jet, std::vector<float> & ePerSampling ) const {

  float emTot=0;
  float hecTot=0;
  float eTot =0;
  size_t numConstit = jet.numConstituents();

  for ( size_t i=0; i<numConstit; i++ ) {
    if (jet.rawConstituent(i)->type()==xAOD::Type::ParticleFlow){
      const xAOD::PFO* constit = static_cast<const xAOD::PFO*>(jet.rawConstituent(i));
      if ( fabs(constit->charge())>FLT_MIN ){
        eTot += constit->track(0)->e();
      } else {
        eTot += constit->e();
        FillESamplingPFO(PreSamplerB);
        FillESamplingPFO(EMB1);
        FillESamplingPFO(EMB2);
        FillESamplingPFO(EMB3);

        FillESamplingPFO(PreSamplerE);
        FillESamplingPFO(EME1);
        FillESamplingPFO(EME2);
        FillESamplingPFO(EME3);

        FillESamplingPFO(HEC0);
        FillESamplingPFO(HEC1);
        FillESamplingPFO(HEC2);
        FillESamplingPFO(HEC3);

        FillESamplingPFO(TileBar0);
        FillESamplingPFO(TileBar1);
        FillESamplingPFO(TileBar2);

        FillESamplingPFO(TileGap1);
        FillESamplingPFO(TileGap2);
        FillESamplingPFO(TileGap3);

        FillESamplingPFO(TileExt0);
        FillESamplingPFO(TileExt1);
        FillESamplingPFO(TileExt2);

        FillESamplingPFO(FCAL0);
        FillESamplingPFO(FCAL1);
        FillESamplingPFO(FCAL2);

        FillESamplingPFO(MINIFCAL0);
        FillESamplingPFO(MINIFCAL1);
        FillESamplingPFO(MINIFCAL2);
        FillESamplingPFO(MINIFCAL3);        

        emTot += ( E_PreSamplerB+E_EMB1+E_EMB2+E_EMB3+
                  E_PreSamplerE+E_EME1+E_EME2+E_EME3+
                  E_FCAL0 );
        hecTot += ( E_HEC0+E_HEC1+E_HEC2+E_HEC3 );
        
      }//only consider neutral PFO
    } else {
      ATH_MSG_WARNING("Tried to call fillEperSamplingPFlow with a jet constituent that is not a PFO!");
    }
  }

  SG::WriteDecorHandle<xAOD::JetContainer, float> emFracHandle(m_emFracKey);
  if(eTot != 0.0){
    emFracHandle(jet) = emTot/eTot; 
    /*
     * Ratio of EM layer calorimeter energy of neutrals to sum of all constituents 
     * at EM scale (note charged PFO have an EM scale at track scale, and charged weights are ignored)
     * */
  }
  else {
    emFracHandle(jet)  = 0.;
  }

  SG::WriteDecorHandle<xAOD::JetContainer, float> hecFracHandle(m_hecFracKey);
  if (eTot != 0.0){
    hecFracHandle(jet) = hecTot/eTot;
  }
  else{
    hecFracHandle(jet) = 0.;
  }
  
}

// R21 way of calculating energy-per-layer using directly the FE decorations
// In R21, the link from PFOs to clusters was broken and thus energy after charged subtraction was used
void JetCaloEnergies::fillEperSamplingFE(const xAOD::Jet& jet, std::vector<float> & ePerSampling ) const {
  float emTot = 0.;
  float em3Tot = 0.;
  float hecTot = 0.;
  float psTot = 0.;
  float tile0Tot = 0.;
  float eTot = 0.;
  float e2Tot = 0.;
  size_t numConstit = jet.numConstituents();    

  std::vector<int> indicesNeutralFE;
  std::vector<int> indicesChargedFE;

  for ( size_t i=0; i<numConstit; i++ ) {
    if(jet.rawConstituent(i)->type()!=xAOD::Type::FlowElement) {
      ATH_MSG_WARNING("Tried to call fillEperSamplingFE with a jet constituent that is not a FlowElement!");
      continue;
    }
    const xAOD::FlowElement* constit = static_cast<const xAOD::FlowElement*>(jet.rawConstituent(i));

    // Need to distinguish two cases:
    // (1) Jet is a PFlow jet (constituents are charged or neutral FEs) or
    // (2) Jet is a UFO jet (need to get the underlying charged and neutral FEs first)

    //For PFlow jets, we can directly get the information from the constituent
    if(constit->signalType() & xAOD::FlowElement::PFlow){

      //Charged FlowElements:
      if(constit->isCharged()){
	eTot += constit->chargedObject(0)->e();
        e2Tot += constit->chargedObject(0)->e()*constit->chargedObject(0)->e();
      }
      //Neutral FlowElements
      else{
        eTot += constit->e();
        e2Tot += constit->e()*constit->e();
        //Get the energy-per-layer information from the FE, not the underlying cluster (i.e. after subtraction)
        std::vector<float> constitEPerSampling = FEHelpers::getEnergiesPerSampling(*constit);
        for ( size_t s = CaloSampling::PreSamplerB; s < CaloSampling::Unknown; s++ ) {
          ePerSampling[s] += constitEPerSampling[s];
        }
        emTot += (constitEPerSampling[CaloSampling::PreSamplerB] + constitEPerSampling[CaloSampling::EMB1]
            + constitEPerSampling[CaloSampling::EMB2]        + constitEPerSampling[CaloSampling::EMB3]
            + constitEPerSampling[CaloSampling::PreSamplerE] + constitEPerSampling[CaloSampling::EME1]
            + constitEPerSampling[CaloSampling::EME2]        + constitEPerSampling[CaloSampling::EME3]
            + constitEPerSampling[CaloSampling::FCAL0]);

        hecTot += (constitEPerSampling[CaloSampling::HEC0] + constitEPerSampling[CaloSampling::HEC1]
            + constitEPerSampling[CaloSampling::HEC2] + constitEPerSampling[CaloSampling::HEC3]);

        psTot += (constitEPerSampling[CaloSampling::PreSamplerB] + constitEPerSampling[CaloSampling::PreSamplerE]);

        em3Tot += (constitEPerSampling[CaloSampling::EMB3] + constitEPerSampling[CaloSampling::EME3]);

        tile0Tot += (constitEPerSampling[CaloSampling::TileBar0] + constitEPerSampling[CaloSampling::TileExt0]);
      }
    }
    else{
      //For UFO jets, we first need to get the charged and neutral FE + corresponding energy from combined UFOs

      // UFO is simply a charged FlowElement
      if(constit->signalType() == xAOD::FlowElement::Charged){
	eTot += constit->chargedObject(0)->e();
        e2Tot += constit->chargedObject(0)->e()*constit->chargedObject(0)->e();
      }
      //UFO is simply a neutral Flowelement
      else if(constit->signalType() == xAOD::FlowElement::Neutral){
        // For neutral UFOs, there is only one "other object" stored which is the neutral FE
        // Protection in case there is something wrong with this FE
        if(constit->otherObjects().size() != 1 || !constit->otherObject(0)){
          continue;
        }

        // Cast other object as FlowElement
        const xAOD::FlowElement* nFE = static_cast<const xAOD::FlowElement*>(constit->otherObject(0));

        eTot += nFE->e();
        e2Tot += nFE->e()*nFE->e();

        std::vector<float> neutralEPerSampling = FEHelpers::getEnergiesPerSampling(*nFE);
        for ( size_t s = CaloSampling::PreSamplerB; s < CaloSampling::Unknown; s++ ) {
          ePerSampling[s] += neutralEPerSampling[s];
        }
        emTot += (neutralEPerSampling[CaloSampling::PreSamplerB] + neutralEPerSampling[CaloSampling::EMB1]
                  + neutralEPerSampling[CaloSampling::EMB2]        + neutralEPerSampling[CaloSampling::EMB3]
                  + neutralEPerSampling[CaloSampling::PreSamplerE] + neutralEPerSampling[CaloSampling::EME1]
                  + neutralEPerSampling[CaloSampling::EME2]        + neutralEPerSampling[CaloSampling::EME3]
                  + neutralEPerSampling[CaloSampling::FCAL0]);

        hecTot += (neutralEPerSampling[CaloSampling::HEC0] + neutralEPerSampling[CaloSampling::HEC1]
                  + neutralEPerSampling[CaloSampling::HEC2] + neutralEPerSampling[CaloSampling::HEC3]);

        psTot += (neutralEPerSampling[CaloSampling::PreSamplerB] + neutralEPerSampling[CaloSampling::PreSamplerE]);

        em3Tot += (neutralEPerSampling[CaloSampling::EMB3] + neutralEPerSampling[CaloSampling::EME3]);

        tile0Tot += (neutralEPerSampling[CaloSampling::TileBar0] + neutralEPerSampling[CaloSampling::TileExt0]);
      }
      else if(constit->signalType() == xAOD::FlowElement::Combined){
        // For the combined UFOs, otherObjects are neutral or charged FEs
        // matched to this tracks (via track-to-cluster extrapolation)
        for (size_t n = 0; n < constit->otherObjects().size(); ++n) {
          if(! constit->otherObject(n)) continue;
          const xAOD::FlowElement* FE_from_combined = static_cast<const xAOD::FlowElement*>(constit->otherObject(n));

          //Charged FE (add energy to total energy only)
          if(FE_from_combined->isCharged()){
            if(std::find(indicesChargedFE.begin(), indicesChargedFE.end(), FE_from_combined->index()) == indicesChargedFE.end()){
              eTot += FE_from_combined->e();
              e2Tot += FE_from_combined->e()*FE_from_combined->e();
              indicesChargedFE.push_back(FE_from_combined->index());
            }
            continue;
          }
          //Neutral FE:
          //One neutral FE can be matched to various tracks and therefore be used for several UFOs
          //We do not want to double count the energy and only add it once
          if(std::find(indicesNeutralFE.begin(), indicesNeutralFE.end(), FE_from_combined->index()) == indicesNeutralFE.end()){
            eTot += FE_from_combined->e();
            e2Tot += FE_from_combined->e()*FE_from_combined->e();
            std::vector<float> neutralFromCombEPerSampling = FEHelpers::getEnergiesPerSampling(*FE_from_combined);
            for ( size_t s = CaloSampling::PreSamplerB; s < CaloSampling::Unknown; s++ ) {
              ePerSampling[s] += neutralFromCombEPerSampling[s];
            }
            emTot += (neutralFromCombEPerSampling[CaloSampling::PreSamplerB] + neutralFromCombEPerSampling[CaloSampling::EMB1]
                + neutralFromCombEPerSampling[CaloSampling::EMB2]        + neutralFromCombEPerSampling[CaloSampling::EMB3]
                + neutralFromCombEPerSampling[CaloSampling::PreSamplerE] + neutralFromCombEPerSampling[CaloSampling::EME1]
                + neutralFromCombEPerSampling[CaloSampling::EME2]        + neutralFromCombEPerSampling[CaloSampling::EME3]
                + neutralFromCombEPerSampling[CaloSampling::FCAL0]);

            hecTot += (neutralFromCombEPerSampling[CaloSampling::HEC0] + neutralFromCombEPerSampling[CaloSampling::HEC1]
                + neutralFromCombEPerSampling[CaloSampling::HEC2] + neutralFromCombEPerSampling[CaloSampling::HEC3]);

            psTot += (neutralFromCombEPerSampling[CaloSampling::PreSamplerB] + neutralFromCombEPerSampling[CaloSampling::PreSamplerE]);

            em3Tot += (neutralFromCombEPerSampling[CaloSampling::EMB3] + neutralFromCombEPerSampling[CaloSampling::EME3]);

            tile0Tot += (neutralFromCombEPerSampling[CaloSampling::TileBar0] + neutralFromCombEPerSampling[CaloSampling::TileExt0]);

            indicesNeutralFE.push_back(FE_from_combined->index());
          }
        }
      }
    }
  }

  double fracSamplingMax = -999999999.;
  int fracSamplingMaxIndex = -1;
  double sumE_samplings = 0.0;

  if(m_doFracSamplingMax){
    for(unsigned int i = 0; i < ePerSampling.size(); ++i){
      double e = ePerSampling[i];
      sumE_samplings += e;
      if (e>fracSamplingMax){
        fracSamplingMax=e;
        fracSamplingMaxIndex = i;
      }
    }
  }

  for( const std::string & calcN : m_calculationNames){
    if ( calcN == "EMFrac" ) {
      SG::WriteDecorHandle<xAOD::JetContainer, float> emFracHandle(m_emFracKey);
      emFracHandle(jet)  = eTot != 0. ? emTot/eTot  : 0.;
    } else if ( calcN == "HECFrac" ) {
      SG::WriteDecorHandle<xAOD::JetContainer, float> hecFracHandle(m_hecFracKey);
      hecFracHandle(jet) = eTot != 0. ? hecTot/eTot : 0.;
    } else if ( calcN == "PSFrac" ) {
      SG::WriteDecorHandle<xAOD::JetContainer, float> psFracHandle(m_psFracKey);
      psFracHandle(jet)  = eTot != 0. ? psTot/eTot  : 0.;
    } else if ( calcN == "EM3Frac" ) {
      SG::WriteDecorHandle<xAOD::JetContainer, float> em3FracHandle(m_em3FracKey);
      em3FracHandle(jet)  = eTot != 0. ? em3Tot/eTot  : 0.;
    } else if ( calcN == "Tile0Frac" ) {
      SG::WriteDecorHandle<xAOD::JetContainer, float> tile0FracHandle(m_tile0FracKey);
      tile0FracHandle(jet)  = eTot != 0. ? tile0Tot/eTot  : 0.;
    } else if ( calcN == "EffNClusts" ) {
      SG::WriteDecorHandle<xAOD::JetContainer, float> effNClustsFracHandle(m_effNClustsFracKey);
      effNClustsFracHandle(jet)  = eTot != 0. ? std::sqrt(eTot*eTot/e2Tot)  : 0.;
    } else if ( calcN == "FracSamplingMax" ){
      SG::WriteDecorHandle<xAOD::JetContainer, float> fracSamplingMaxHandle(m_fracSamplingMaxKey);
      fracSamplingMaxHandle(jet) = sumE_samplings != 0. ? fracSamplingMax/sumE_samplings : 0.;
      SG::WriteDecorHandle<xAOD::JetContainer, int> fracSamplingMaxIndexHandle(m_fracSamplingMaxIndexKey);
      fracSamplingMaxIndexHandle(jet) = fracSamplingMaxIndex;
    }
  }
}


// New way of calculating energy per layer for FE-based jets
// The underlying cluster is used to calculate the variables rather than the energy-subtracted FE
void JetCaloEnergies::fillEperSamplingFEClusterBased(const xAOD::Jet& jet, std::vector<float> & ePerSampling ) const {
  float emTot = 0.;
  float em3Tot = 0.;
  float hecTot = 0.;
  float psTot = 0.;
  float tile0Tot = 0.;
  float eTot = 0.;
  float e2Tot = 0.;
  float sumRadialDistanceSquared = 0;
  float sumLongitudinalDistanceSquared = 0;
  size_t numConstit = jet.numConstituents();
  std::unique_ptr<std::vector<const xAOD::CaloCluster*> > constitV_tot = std::make_unique<std::vector<const xAOD::CaloCluster*>>();
  const xAOD::CaloCluster* leadingCluster = nullptr;

  for ( size_t i=0; i<numConstit; i++ ) {
    if(jet.rawConstituent(i)->type()!=xAOD::Type::FlowElement) {
      ATH_MSG_WARNING("Tried to call fillEperSamplingFE with a jet constituent that is not a FlowElement!");
      continue;
    }
    const xAOD::FlowElement* constit = static_cast<const xAOD::FlowElement*>(jet.rawConstituent(i));

    for (size_t n = 0; n < constit->otherObjects().size(); ++n) {
      if(! constit->otherObject(n)) continue;
      int index_pfo = constit->otherObject(n)->index();
      if(index_pfo<0) continue;

      const auto* fe = (constit->otherObject(n));
      const xAOD::CaloCluster* cluster = nullptr;

      //If we have a cluster, we can directly access the calorimeter information
      if(fe->type() == xAOD::Type::CaloCluster){
        cluster = dynamic_cast<const xAOD::CaloCluster*> (fe);
      }
      //If we have a PFO, we should still get the associated cluster first
      else {
        const xAOD::FlowElement* pfo = dynamic_cast<const xAOD::FlowElement*>(fe);
        if(!pfo->otherObjects().empty() && pfo->otherObject(0) && pfo->otherObject(0)->type() == xAOD::Type::CaloCluster){
          cluster = dynamic_cast<const xAOD::CaloCluster*> (pfo->otherObject(0));
        }
      }
      if(!cluster) continue;

      if (!leadingCluster || (cluster->rawE() > leadingCluster->rawE())){
        leadingCluster = cluster;
      }
      
      
      if(std::find(constitV_tot->begin(), constitV_tot->end(), cluster) == constitV_tot->end()){
        for ( size_t s= CaloSampling::PreSamplerB; s< CaloSampling::Unknown; s++ ) {
          ePerSampling[s] += cluster->eSample( (xAOD::CaloCluster::CaloSample) s );
        }
        eTot += cluster->rawE();
        e2Tot += cluster->rawE()*cluster->rawE();

        emTot += (cluster->eSample( CaloSampling::PreSamplerB) + cluster->eSample( CaloSampling::EMB1)
		  + cluster->eSample( CaloSampling::EMB2)        + cluster->eSample( CaloSampling::EMB3)
		  + cluster->eSample( CaloSampling::PreSamplerE) + cluster->eSample( CaloSampling::EME1)
		  + cluster->eSample( CaloSampling::EME2)        + cluster->eSample( CaloSampling::EME3)
		  + cluster->eSample( CaloSampling::FCAL0));

        hecTot += (cluster->eSample( CaloSampling::HEC0) + cluster->eSample( CaloSampling::HEC1)
		   + cluster->eSample( CaloSampling::HEC2) + cluster->eSample( CaloSampling::HEC3));

        psTot += (cluster->eSample( CaloSampling::PreSamplerB) + cluster->eSample( CaloSampling::PreSamplerE));

        em3Tot += (cluster->eSample( CaloSampling::EMB3) + cluster->eSample( CaloSampling::EME3));

        tile0Tot += (cluster->eSample( CaloSampling::TileBar0) + cluster->eSample( CaloSampling::TileExt0));

        sumRadialDistanceSquared += cluster->rawE() * getMoment(cluster, xAOD::CaloCluster::SECOND_R);
        sumLongitudinalDistanceSquared += cluster->rawE() * getMoment(cluster, xAOD::CaloCluster::SECOND_LAMBDA);
         
        constitV_tot->push_back(cluster);
      }
    }
  }

  float fracSamplingMax = -999999999.;
  int fracSamplingMaxIndex = -1;
  float sumE_samplings = 0.0;

  if(m_doFracSamplingMax){
    for(unsigned int i = 0; i < ePerSampling.size(); ++i){
      float e = ePerSampling[i];
      sumE_samplings += e;
      if (e>fracSamplingMax){
	fracSamplingMax=e;
	fracSamplingMaxIndex = i;
      }
    }
  }

  for( const std::string & calcN : m_calculationNames){
    if ( calcN == "EMFrac" ) {
      SG::WriteDecorHandle<xAOD::JetContainer, float> emFracClusterHandle(m_emFracClusterKey);
      emFracClusterHandle(jet)  = eTot != 0. ? emTot/eTot  : 0.;
    } else if ( calcN == "HECFrac" ) {
      SG::WriteDecorHandle<xAOD::JetContainer, float> hecFracClusterHandle(m_hecFracClusterKey);
      hecFracClusterHandle(jet) = eTot != 0. ? hecTot/eTot : 0.;
    } else if ( calcN == "PSFrac" ) {
      SG::WriteDecorHandle<xAOD::JetContainer, float> psFracClusterHandle(m_psFracClusterKey);
      psFracClusterHandle(jet)  = eTot != 0. ? psTot/eTot  : 0.;
    } else if ( calcN == "EM3Frac" ) {
      SG::WriteDecorHandle<xAOD::JetContainer, float> em3FracClusterHandle(m_em3FracClusterKey);
      em3FracClusterHandle(jet)  = eTot != 0. ? em3Tot/eTot  : 0.;
    } else if ( calcN == "Tile0Frac" ) {
      SG::WriteDecorHandle<xAOD::JetContainer, float> tile0FracClusterHandle(m_tile0FracClusterKey);
      tile0FracClusterHandle(jet)  = eTot != 0. ? tile0Tot/eTot  : 0.;
    } else if ( calcN == "EffNClusts" ) {
      SG::WriteDecorHandle<xAOD::JetContainer, float> effNClustsFracClusterHandle(m_effNClustsFracClusterKey);
      effNClustsFracClusterHandle(jet)  = eTot != 0. ? std::sqrt(eTot*eTot/e2Tot)  : 0.;
    } else if ( calcN == "FracSamplingMax" ){
      SG::WriteDecorHandle<xAOD::JetContainer, float> fracSamplingMaxClusterHandle(m_fracSamplingMaxClusterKey);
      fracSamplingMaxClusterHandle(jet) = sumE_samplings != 0. ? fracSamplingMax/sumE_samplings : 0.;
      SG::WriteDecorHandle<xAOD::JetContainer, int> fracSamplingMaxIndexClusterHandle(m_fracSamplingMaxIndexClusterKey);
      fracSamplingMaxIndexClusterHandle(jet) = fracSamplingMaxIndex;
    }
  }

  SG::WriteDecorHandle<xAOD::JetContainer, float> lambdaLeadingClusterHandle(m_lambdaLeadingClusterKey);
  lambdaLeadingClusterHandle(jet) = getMoment(leadingCluster, xAOD::CaloCluster::CENTER_LAMBDA);

  SG::WriteDecorHandle<xAOD::JetContainer, float> meanRadialDistanceSquaredHandle(m_meanRadialDistanceSquaredKey);
  meanRadialDistanceSquaredHandle(jet) = eTot != 0. ? sumRadialDistanceSquared/eTot  : 0.;

  SG::WriteDecorHandle<xAOD::JetContainer, float> meanLongitudinalDistanceSquaredHandle(m_meanLongitudinalDistanceSquaredKey);
  meanLongitudinalDistanceSquaredHandle(jet) = eTot != 0. ? sumLongitudinalDistanceSquared/eTot  : 0.;
}

float JetCaloEnergies::getMoment(const xAOD::CaloCluster* cluster, const xAOD::CaloCluster::MomentType& momentType) const {
    if (cluster){
        double moment = 0.0;
        bool isRetrieved = cluster->retrieveMoment(momentType, moment);
        if (isRetrieved) return (float) moment;
    }
    ATH_MSG_DEBUG("Can not retrieve moment from cluster");
    return 0.0;
}
