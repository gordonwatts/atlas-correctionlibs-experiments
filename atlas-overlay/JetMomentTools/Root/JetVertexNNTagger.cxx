/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "JetMomentTools/JetVertexNNTagger.h"

#include <fstream>
#include <regex>
#include <map>

#include "AsgDataHandles/ReadHandle.h"
#include "xAODJet/JetContainer.h"
#include "xAODJet/JetAuxContainer.h"
#include "xAODBase/IParticleHelpers.h"
#include "xAODCore/ShallowCopy.h"
#include "PathResolver/PathResolver.h"

#include "AsgDataHandles/ReadDecorHandle.h"
#include "AsgDataHandles/WriteDecorHandle.h"

#include "nn_graph.hxx"
#include "lwtnn/parse_json.hh"
#include "lwtnn/Stack.hh"

using xAOD::JetContainer;

namespace JetPileupTag {

    JetVertexNNTagger::JetVertexNNTagger(const std::string& name)
    : asg::AsgTool(name) {}

    JetVertexNNTagger::~JetVertexNNTagger() = default;


    StatusCode JetVertexNNTagger::initialize()
    {

        ATH_MSG_DEBUG("Initializing...");

        if(m_jetContainerName.empty()){
            ATH_MSG_ERROR("JetVertexTaggerTool needs to have its input jet container configured!");
            return StatusCode::FAILURE;
        }


        // Determine which NNJVT configuration to use based on the flag
        std::string configDir;
        std::string paramFileName;
        std::string cutFileName;

        if (m_useTrkAugNN) {
            ATH_MSG_INFO("Using Track-Augmented NN configuration.");
            configDir = m_TrkAugNNConfigDir;
            paramFileName = m_TrkAugNNParamFileName;
            cutFileName = m_TrkAugNNCutFileName;
        } else {
            ATH_MSG_INFO("Using Classic NN configuration.");
            configDir = m_NNConfigDir;
            paramFileName = m_NNParamFileName;
            cutFileName = m_NNCutFileName;
        }


        // Use the Path Resolver to find the jvt file and retrieve the likelihood histogram
        std::string configPath = PathResolverFindCalibFile(configDir+"/"+paramFileName);
        ATH_MSG_INFO("Reading JVT NN file from: " << paramFileName);
        ATH_MSG_DEBUG("                     resolved in  :\n    " << configPath << "\n\n");

        std::ifstream fconfig( configPath.c_str() );
        if ( !fconfig.is_open() ) {
        ATH_MSG_ERROR( "Error opening config file: " << m_NNParamFileName );
        ATH_MSG_ERROR( "Are you sure that the file exists at this path?" );
        return StatusCode::FAILURE;
        }


        ATH_MSG_INFO("\n Reading JVT likelihood histogram from:\n    " << configPath << "\n\n");
        lwt::GraphConfig cfg = lwt::parse_json_graph( fconfig );

        // Determine which NN input features to use based on the flag
        std::vector<std::string> inputs;
        if (m_useTrkAugNN) {
            // Input features are identical between offline and HLT but the model files for HLT use a different naming convention
            if (paramFileName.find("HLT") != std::string::npos)
                inputs = std::vector<std::string>{"pt", "eta", "NumTrkPt1000_vx0", "TrackWidthPt1000_vx0", "RPtTrkPt500_vx0", "DNumTrkPt1000_vx1", "DTrackWidthPt1000_vx1", "DRPtTrkPt500_vx1"};
            else
                inputs = std::vector<std::string>{"pt", "eta", "SumPtTrkOrderedNumTrkPt1000_vx0", "SumPtTrkOrderedTrackWidthPt1000_vx0", "SumPtTrkOrderedRPtTrkPt500_vx0", "DNumTrkPt1000_vx1", "DTrackWidthPt1000_vx1", "DRPtTrkPt500_vx1"};
        }
        else {
            inputs = std::vector<std::string>{"Rpt", "JVFCorr", "ptbin", "etabin"};
        }

        // FastGraph is initialised with the order in which inputs will be
        // provided, to avoid map lookup
        lwt::InputOrder order;
        lwt::order_t node_order;

        // Single input block
        node_order.emplace_back(cfg.inputs[0].name,inputs);
        order.scalar = node_order;
        ATH_MSG_DEBUG( "Network NLayers: " << cfg.layers.size() );

        // Primarily using double to take advantage of build_vector later
        m_lwnn = std::make_unique<CachedNNGraph>(cfg,order);

        std::string cutsPath = PathResolverFindCalibFile(configDir+"/"+cutFileName);
        ATH_MSG_INFO("Reading JVT NN cut file from: " << cutFileName);
        ATH_MSG_DEBUG("                     resolved in  :\n    " << cutsPath << "\n\n");
        std::ifstream fcuts( cutsPath.c_str() );
        if ( !fcuts.is_open() ) {
            ATH_MSG_ERROR( "Error opening cuts file: " << cutFileName );
            ATH_MSG_ERROR( "Are you sure that the file exists at this path?" );
            return StatusCode::FAILURE;
        }
        m_cutMap = NNJvtCutMap::fromJSON(fcuts);

        m_jvfCorrKey = m_jetContainerName + "." + m_jvfCorrKey.key();
        m_sumPtTrkKey = m_jetContainerName + "." + m_sumPtTrkKey.key();
        m_trkWidthKey = m_jetContainerName + "." + m_trkWidthKey.key();
        m_numTrkKey = m_jetContainerName + "." + m_numTrkKey.key();
        m_jvtKey = m_jetContainerName + "." + m_jvtKey.key();
        if (!m_rptKey.empty())
            m_rptKey = m_jetContainerName + "." + m_rptKey.key();
        if (!m_passJvtKey.empty())
            m_passJvtKey = m_jetContainerName + "." + m_passJvtKey.key();
        m_rptPerVertexKey = m_jetContainerName + "." + m_rptPerVertexKey.key();
        m_DtrkWidthKey = m_jetContainerName + "." + m_DtrkWidthKey.key();
        m_DnumTrkKey = m_jetContainerName + "." + m_DnumTrkKey.key();
        m_DrptPerVertexKey = m_jetContainerName + "." + m_DrptPerVertexKey.key();
        m_TrkWidthSortedKey = m_jetContainerName + "." + m_TrkWidthSortedKey.key();
        m_NumTrkSortedKey = m_jetContainerName + "." + m_NumTrkSortedKey.key();


    #ifndef XAOD_STANDALONE
        if (m_suppressInputDeps) {
            // Suppress input dependencies
            renounce(m_jvfCorrKey);
            renounce(m_sumPtTrkKey);
            renounce(m_trkWidthKey);
            renounce(m_numTrkKey);


        }
        if (m_suppressOutputDeps) {
            // Suppress output dependencies
            renounce(m_jvtKey);
            renounce(m_rptKey);
            renounce(m_passJvtKey);
            if (m_useTrkAugNN) {
                renounce(m_rptPerVertexKey);
                renounce(m_DtrkWidthKey);
                renounce(m_DnumTrkKey);
                renounce(m_DrptPerVertexKey);
                renounce(m_TrkWidthSortedKey);
                renounce(m_NumTrkSortedKey);
            }

        }
    #endif

        ATH_CHECK(m_vertexContainer_key.initialize());
        ATH_CHECK(m_jvfCorrKey.initialize(!m_useTrkAugNN));
        ATH_CHECK(m_sumPtTrkKey.initialize());
        ATH_CHECK(m_trkWidthKey.initialize(m_useTrkAugNN));
        ATH_CHECK(m_numTrkKey.initialize(m_useTrkAugNN));
        ATH_CHECK(m_jvtKey.initialize());
        ATH_CHECK(m_rptKey.initialize(SG::AllowEmpty));
        ATH_CHECK(m_passJvtKey.initialize(SG::AllowEmpty));
        ATH_CHECK(m_rptKey.initialize(SG::AllowEmpty));
        ATH_CHECK(m_rptPerVertexKey.initialize(m_useTrkAugNN));
        ATH_CHECK(m_DtrkWidthKey.initialize(m_useTrkAugNN));
        ATH_CHECK(m_DnumTrkKey.initialize(m_useTrkAugNN));
        ATH_CHECK(m_DrptPerVertexKey.initialize(m_useTrkAugNN));
        ATH_CHECK(m_TrkWidthSortedKey.initialize(m_useTrkAugNN));
        ATH_CHECK(m_NumTrkSortedKey.initialize(m_useTrkAugNN));

        return StatusCode::SUCCESS;
    }


    float JetVertexNNTagger::evaluateJvt(const std::vector<float>& features) const
    {
        std::vector<double> features_double(features.begin(), features.end());

        // Convert inputs to lwt::VectorX format
        lwt::VectorX<double> inputvals = lwt::build_vector(features_double);

        // Wrap inputs into a vector of scalars as required by FastGraph
        std::vector<lwt::VectorX<double>> scalars{inputvals};

        // Compute the output using the NN model
        lwt::VectorX<double> output = m_lwnn->compute(scalars);

        // Return the first output node value
        return output(0);
    }


    const xAOD::Vertex *JetVertexNNTagger::findHSVertex(const xAOD::VertexContainer& vertices) const
    {

        for ( const xAOD::Vertex* vertex : vertices ) {
            if(vertex->vertexType() == xAOD::VxType::PriVtx) {
                ATH_MSG_VERBOSE("JetVertexTaggerTool " << name() << " Found HS vertex at index: "<< vertex->index());
                return vertex;
            }
        }
        if (vertices.size()==1) {
            ATH_MSG_VERBOSE("JetVertexTaggerTool " << name() << " Found no HS vertex, return dummy");
            if (vertices.back()->vertexType() == xAOD::VxType::NoVtx)
                return vertices.back();
        }
        ATH_MSG_VERBOSE("No vertex found in container.");
        return nullptr;
    }


    // resort vertices
    std::vector<size_t> JetVertexNNTagger::get_resorted_vertex_indices(const std::vector<float>& jet_sumpt_per_vertex, const xAOD::Vertex* HSvertex) const
    {

        ATH_MSG_VERBOSE("Starting to create pair of jet sumpt and its index, excluding the HS vertex (index 0).");


        //  Make pair with first value = sumpt value, second = index of value
        // *Exclude the HS vertex*

        size_t HS_index = HSvertex->index();

        // Handle cases with only one vertex
        if (jet_sumpt_per_vertex.size() <= 1) {
            ATH_MSG_VERBOSE("Only one vertex available. Returning the single vertex index.");
            return { HS_index };
        }

        std::vector<std::pair<float, size_t>> pairedValues;

        for (size_t i = 0; i < jet_sumpt_per_vertex.size(); ++i) {
            if (i != HS_index) {
                pairedValues.emplace_back(jet_sumpt_per_vertex[i], i);
            }
        }

        ATH_MSG_VERBOSE("Finished creating paired values, now sorting them by sumpt values.");


        // Sort pairs by first sumpt value
	    std::sort(pairedValues.begin(), pairedValues.end(),
		      [](const std::pair<float, size_t>& a, const std::pair<float, size_t>& b) {
		          return a.first > b.first;  // Sort in descending order of sumpt
		      });


        ATH_MSG_VERBOSE("Sorting complete. Extracting indices now.");

        std::vector<size_t> resorted_vertex_indices;

        // insert HS vertex index at position 0
        resorted_vertex_indices.push_back(HS_index);

        // Extract indices from the sorted pairedValues
        for (const auto& pair : pairedValues) {
            resorted_vertex_indices.push_back(pair.second);
        }

        ATH_MSG_VERBOSE("Finished extracting indices. Returning the final resorted indices vector.");


        return  resorted_vertex_indices;
    }


    JetVertexNNTagger::OrderedTrackMoment JetVertexNNTagger::get_sorted_track_moments(
        const TrackMomentStruct& moment, const std::vector<size_t>& resorted_vertex_indices, float jetPt, float invalidRpt) const
            {
                std::vector<OrderedTrackMoment> orderedMoments;

                std::vector<int> numTrk_sorted;
                std::vector<float> trkWidth_sorted;
                std::vector<float> rpt_sorted;


                if (resorted_vertex_indices.empty()) {
                    ATH_MSG_WARNING("resorted_vertex_indices is empty, returning empty moment.");
                    return {};
                }

                // Loop through each vertex index
                for (size_t i = 0; i < resorted_vertex_indices.size(); ++i) {
                    size_t original_index = resorted_vertex_indices[i];

                    if (original_index >= moment.numTrk.size()) {
                        ATH_MSG_WARNING("Index out of range in resorted_vertex_indices: " << original_index);
                        continue;
                    }

                    // Add the sorted values for each vertex
                    numTrk_sorted.push_back(moment.numTrk[original_index]);
                    trkWidth_sorted.push_back(moment.trkWidth[original_index]);

                    // Calculate rpt and add to the sorted rpt vector
                    float rpt = (jetPt != 0) ? (moment.sumPtTrk[original_index] / jetPt) : invalidRpt;
                    rpt_sorted.push_back(rpt);

                }

                return OrderedTrackMoment(numTrk_sorted, trkWidth_sorted, rpt_sorted);

            }


    template <typename T>
    std::string vectorToString(const std::vector<T>& vec) {
        std::ostringstream oss;
        for (const auto& val : vec) {
            oss << val << " ";
        }
        return oss.str();
    }

    // calculate difference between jet moments for the leading and subleading vertices


    JetVertexNNTagger::DTrackMomentStruct JetVertexNNTagger::calculatePerVertexDifferences(
        const OrderedTrackMoment& sortedTrackMoments, size_t numVertices) const
    {


        // Ensure there are enough vertices for computation
        if (sortedTrackMoments.numTrk.empty() || numVertices <= 1) {
            ATH_MSG_WARNING("Moments are empty or numVertices is too small, returning empty struct.");
            return DTrackMomentStruct(std::vector<int>(numVertices - 1, 0),
                                    std::vector<float>(numVertices - 1, 0.0f),
                                    std::vector<float>(numVertices - 1, 0.0f));
        }

        size_t maxVertices = std::min(numVertices, sortedTrackMoments.numTrk.size());

        std::vector<int> dNumTrk(numVertices - 1, 0);
        std::vector<float> dTrkWidth(numVertices - 1, 0.0f);
        std::vector<float> dRpt(numVertices - 1, 0.0f);

        // Set the leading values (first element in the sorted track moments)
        int leadingNumTrk = sortedTrackMoments.numTrk[0];
        float leadingTrkWidth = sortedTrackMoments.trkWidth[0];
        float leadingRpt = sortedTrackMoments.rpt[0];

        // Compute the differences for each vertex
        for (size_t ivx = 1; ivx < maxVertices; ++ivx) {
            dNumTrk[ivx - 1] = leadingNumTrk - sortedTrackMoments.numTrk[ivx];
            dTrkWidth[ivx - 1] = leadingTrkWidth - sortedTrackMoments.trkWidth[ivx];
            dRpt[ivx - 1] = leadingRpt - sortedTrackMoments.rpt[ivx];
        }

        return DTrackMomentStruct(dNumTrk, dTrkWidth, dRpt);
    }



    StatusCode JetVertexNNTagger::decorate(const xAOD::JetContainer& jetCont) const
    {

        // Grab vertices for index bookkeeping
        SG::ReadHandle<xAOD::VertexContainer> vertexHandle = SG::makeHandle (m_vertexContainer_key);
        const xAOD::VertexContainer& vertices = *vertexHandle;
        ATH_MSG_DEBUG("Successfully retrieved VertexContainer: " << m_vertexContainer_key.key());

        const xAOD::Vertex *HSvertex = findHSVertex(vertices);
        if(!HSvertex) {
            ATH_MSG_WARNING("Invalid primary vertex found, will not continue decorating with JVT.");
            return StatusCode::FAILURE;
        }

        ATH_MSG_DEBUG("Primary vertex found with index: " << HSvertex->index());


        SG::WriteDecorHandle<xAOD::JetContainer, float> jvtHandle(m_jvtKey);
        SG::WriteDecorHandle<xAOD::JetContainer, char>  passJvtHandle(m_passJvtKey);


        std::unique_ptr<TrkAugHandleHolder> trkAugHandleHolder;
        std::unique_ptr<ClassicHandleHolder> classicHandleHolder;

        if (m_useTrkAugNN) {
            trkAugHandleHolder = std::make_unique<TrkAugHandleHolder>(
                m_trkWidthKey, m_sumPtTrkKey, m_numTrkKey,
                m_DtrkWidthKey, m_DnumTrkKey, m_DrptPerVertexKey,
                m_rptPerVertexKey, m_TrkWidthSortedKey, m_NumTrkSortedKey
            );
        } else {
            classicHandleHolder = std::make_unique<ClassicHandleHolder>(
                m_jvfCorrKey, m_sumPtTrkKey, m_rptKey
            );
        }



        static constexpr float invalidJvt = -1;
        static constexpr float invalidRpt = 0;
        static constexpr char invalidPassJvt = true;

        for(const xAOD::Jet* jet : jetCont) {
            float jvt = invalidJvt;
            float rpt = invalidRpt;
            char passJvt = invalidPassJvt;

            // consider only leading numVertices for difference calucations.
            //Vertices ordered acccording to descending SumTrkPt_vxi, for i>0. SumTrkPt_vx0 is for HS vertex.
            size_t numVertices = 4;

            std::vector<float> sortedTrkWidthVec, rptVec, dTrkWidthVec, dRptVec;
            std::vector<int> sortedNumTrkVec, dNumTrkVec;

            if (HSvertex->vertexType() == xAOD::VxType::PriVtx) {


                ATH_MSG_VERBOSE("Processing jet with pt: " << jet->pt() << " and eta: " << jet->eta());


                size_t ptbin, etabin;


                std::vector<float> features;


                if (m_useTrkAugNN) {

                    TrackMomentStruct trackMoment = trkAugHandleHolder->getTrackMoments(*jet);

                    std::vector<float> trkwidth = trkAugHandleHolder->trkWidthHandle(*jet);

                    std::vector<int> numtrk = trkAugHandleHolder->numTrkHandle(*jet);

                    std::vector<float> sumpttrk = trkAugHandleHolder->sumPtTrkHandle(*jet);



                    //TrackMomentStruct trackMoment(numtrk, trkwidth, sumpttrk);

                    ATH_MSG_VERBOSE("Before sorting:");
                    ATH_MSG_VERBOSE("sumpttrk: " << vectorToString(sumpttrk));
                    ATH_MSG_VERBOSE("trkwidth: " << vectorToString(trkwidth));
                    ATH_MSG_VERBOSE("numtrk: " << vectorToString(numtrk));


                    // Get resorted vertex indices based on sumpttrk values
                    std::vector<size_t> resorted_vertex_indices = get_resorted_vertex_indices(sumpttrk, HSvertex);

                    if (resorted_vertex_indices.empty()) {
                        ATH_MSG_WARNING("resorted_vertex_indices is EMPTY for jet with pt: " << jet->pt());
                        continue;
                    }

                    // Get sorted track moments with rpt included and sorted
                    OrderedTrackMoment sortedTrackMoments = get_sorted_track_moments(trackMoment, resorted_vertex_indices, jet->pt(), invalidRpt);

                    ATH_MSG_VERBOSE("After sorting:");
                    ATH_MSG_VERBOSE("numTrk: " << vectorToString(sortedTrackMoments.numTrk));
                    ATH_MSG_VERBOSE("trkWidth: " << vectorToString(sortedTrackMoments.trkWidth));
                    ATH_MSG_VERBOSE("rpt: " << vectorToString(sortedTrackMoments.rpt));

                    DTrackMomentStruct differences = calculatePerVertexDifferences(sortedTrackMoments, numVertices);

                    ATH_MSG_VERBOSE("Difference - NumTrk: " << vectorToString(differences.dNumTrk));
                    ATH_MSG_VERBOSE("Difference - TrackWidth: " << vectorToString(differences.dTrkWidth));
                    ATH_MSG_VERBOSE("Difference - RpT: " << vectorToString(differences.dRpt));

                    if (differences.dNumTrk.empty() || differences.dTrkWidth.empty() || differences.dRpt.empty()) {
                        ATH_MSG_WARNING("differences vector is EMPTY!");
                    }

                    rptVec.assign(sortedTrackMoments.rpt.begin(), sortedTrackMoments.rpt.end());
                    sortedTrkWidthVec.assign(sortedTrackMoments.trkWidth.begin(), sortedTrackMoments.trkWidth.end());
                    sortedNumTrkVec.assign(sortedTrackMoments.numTrk.begin(), sortedTrackMoments.numTrk.end());
                    dNumTrkVec.assign(differences.dNumTrk.begin(), differences.dNumTrk.end());
                    dTrkWidthVec.assign(differences.dTrkWidth.begin(), differences.dTrkWidth.end());
                    dRptVec.assign(differences.dRpt.begin(), differences.dRpt.end());

                    if (jet->pt() <= m_maxpt_for_cut && m_cutMap.edges(*jet, ptbin, etabin)) {

                        ATH_MSG_VERBOSE("Defining NNJVT features");

                        // Use Track-Augmented NN features
                        features = { static_cast<float>(jet->pt()),
                                    static_cast<float>(jet->eta()),
                                    static_cast<float>(sortedTrackMoments.numTrk[0]),
                                    sortedTrackMoments.trkWidth[0],
                                    sortedTrackMoments.rpt[0],
                                    static_cast<float>(differences.dNumTrk[0]),
                                    differences.dTrkWidth[0],
                                    differences.dRpt[0] };

                        ATH_MSG_VERBOSE("Using Track-Augmented NN features: "
                            << "jet_pt = " << jet->pt()
                            << ", jet_eta = " << jet->eta()
                            << ", numtrk_sorted[0] = " << sortedTrackMoments.numTrk[0]
                            << ", trkwidth_sorted[0] = " << sortedTrackMoments.trkWidth[0]
                            << ", rpt_per_vertex[0] = " << sortedTrackMoments.rpt[0]
                            << ", dnumtrk[0] = " << differences.dNumTrk[0]
                            << ", dtrkwidth[0] = " << differences.dTrkWidth[0]
                            << ", dRpt_per_vertex[0] = " << differences.dRpt[0]);


                        jvt = evaluateJvt(features);
                        float jvtCut = m_cutMap(ptbin, etabin);
                        passJvt = jvt > jvtCut;

                        ATH_MSG_VERBOSE("Jet with pt " << jet->pt() << ", eta " << jet->eta() );
                        ATH_MSG_VERBOSE("JVT cut for ptbin " << ptbin << ", etabin " << etabin << " = " << jvtCut);
                        ATH_MSG_VERBOSE("Evaluated JVT = " << jvt << ", jet " << (passJvt ? "passes" :"fails") << " working point" );
                    }


                } else {

                    std::vector<float> sumpttrk = classicHandleHolder->sumPtTrkHandle(*jet);

                    if (sumpttrk.empty()) {
                        ATH_MSG_WARNING("sumPtTrkHandle returned an EMPTY vector for jet with pt: " << jet->pt());
                        continue;
                    }


                    // Calculate RpT and JVFCorr
                    float jvfcorr = classicHandleHolder->jvfCorrHandle(*jet);


                    rpt = (jet->pt() != 0) ? (sumpttrk[HSvertex->index() - vertices[0]->index()] / jet->pt()) : invalidRpt;


                    ATH_MSG_VERBOSE("Defining NNJVT features");

                    if (jet->pt() <= m_maxpt_for_cut && m_cutMap.edges(*jet, ptbin, etabin)) {


                        // Use standard NN features
                        features = {rpt, jvfcorr, static_cast<float>(ptbin), static_cast<float>(etabin)};

                        ATH_MSG_VERBOSE("Using standard NN features: "
                                    << "rpt = " << rpt
                                    << ", corrJVF = " << jvfcorr
                                    << ", ptbin = " << ptbin
                                    << ", etabin = " << etabin);

                                            jvt = evaluateJvt(features);

                        float jvtCut = m_cutMap(ptbin, etabin);
                        passJvt = jvt > jvtCut;

                        ATH_MSG_VERBOSE("Jet with pt " << jet->pt() << ", eta " << jet->eta() );
                        ATH_MSG_VERBOSE("JVT cut for ptbin " << ptbin << ", etabin " << etabin << " = " << jvtCut);
                        ATH_MSG_VERBOSE("Evaluated JVT = " << jvt << ", jet " << (passJvt ? "passes" :"fails") << " working point" );
                    }


                }




            }

            // Decorate jet
            jvtHandle(*jet) = jvt;

            if (!passJvtHandle.key().empty())
                passJvtHandle(*jet) = passJvt;


            if (m_useTrkAugNN) {
                trkAugHandleHolder->decorate(*jet, rptVec, dNumTrkVec, dTrkWidthVec, dRptVec, sortedTrkWidthVec, sortedNumTrkVec);
            } else {
                classicHandleHolder->decorate(*jet, rpt);
            }

        }

        ATH_MSG_DEBUG("All jets have been processed for decoration.");


        return StatusCode::SUCCESS;
    }
}


