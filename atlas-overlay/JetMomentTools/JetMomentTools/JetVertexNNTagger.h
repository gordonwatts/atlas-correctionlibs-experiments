// this file is -*- C++ -*-
/*
  Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration
*/

#ifndef JETVERTEXNNTAGGER_H
#define JETVERTEXNNTAGGER_H

///
/// \class JetVertexNNTagger
///
/// Creates a new JetContainer by doing a shallow copy of an input JetVector
///
/// This tool implements the IJetProvider interface. The JetContainer it returns is build by
/// doing a shallow copy of an input JetVector.
/// The JetVector key is also a Property of the tool.
/// 

#include <string>
#include <vector>
#include <utility>
#include <memory>
#include <functional>
#include <optional>

#include "AsgTools/PropertyWrapper.h"
#include "AsgTools/AsgTool.h"
#include "AsgDataHandles/ReadDecorHandle.h"
#include "AsgDataHandles/WriteDecorHandle.h"
#include "AsgDataHandles/ReadHandleKey.h"
#include "AsgDataHandles/ReadDecorHandleKey.h"
#include "AsgDataHandles/WriteDecorHandleKey.h"
#include "JetInterface/IJetDecorator.h"
#include "JetMomentTools/NNJvtBinning.h"

#include "xAODJet/JetContainer.h"
#include "xAODTracking/VertexContainer.h"

class CachedNNGraph;

namespace JetPileupTag {

  // Because there is no SystemOfUnits.h in AnalysisBase
  constexpr float GeV=1e3;

  class JetVertexNNTagger
    : public asg::AsgTool,
      virtual public IJetDecorator
  {
    ASG_TOOL_CLASS(JetVertexNNTagger,IJetDecorator)

    


    public:
      /// Constructor with a tool name
      JetVertexNNTagger(const std::string& name);
      /// Destructor
      ~JetVertexNNTagger();

      // Called in parent initialize()
      virtual StatusCode initialize() override;

      // Inherited method to decorate a jet container
      virtual StatusCode decorate(const xAOD::JetContainer& jetCont) const override;


      struct TrackMomentStruct {
          std::vector<int> numTrk;
          std::vector<float> trkWidth;
          std::vector<float> sumPtTrk;

          TrackMomentStruct(const std::vector<int>& n, const std::vector<float>& w, const std::vector<float>& s)
              : numTrk(n), trkWidth(w), sumPtTrk(s) {}
      };

      struct DTrackMomentStruct {

          std::vector<int> dNumTrk;
          std::vector<float> dTrkWidth;
          std::vector<float> dRpt;

          DTrackMomentStruct(std::vector<int> dN = {}, std::vector<float> dW = {}, std::vector<float> dR = {})
              : dNumTrk(std::move(dN)), dTrkWidth(std::move(dW)), dRpt(std::move(dR)) {}
      };

      struct OrderedTrackMoment {
          std::vector<int> numTrk;
          std::vector<float> trkWidth;
          std::vector<float> rpt;
          

          
          OrderedTrackMoment(std::vector<int> n = {}, std::vector<float> w = {}, std::vector<float> r = {})
              : numTrk(std::move(n)), trkWidth(std::move(w)), rpt(std::move(r)) {}

      };

      struct ClassicHandleHolder {

          SG::ReadDecorHandle<xAOD::JetContainer, float> jvfCorrHandle;
          SG::ReadDecorHandle<xAOD::JetContainer, std::vector<float>> sumPtTrkHandle;
          SG::WriteDecorHandle<xAOD::JetContainer, float> rptHandle;


          ClassicHandleHolder(
              const SG::ReadDecorHandleKey<xAOD::JetContainer>& jvfCorrKey,
              const SG::ReadDecorHandleKey<xAOD::JetContainer>& sumPtTrkKey,
              const SG::WriteDecorHandleKey<xAOD::JetContainer>& rptKey
          ) :
              jvfCorrHandle(jvfCorrKey),
              sumPtTrkHandle(sumPtTrkKey),
              rptHandle(rptKey)
          {}
          
          void decorate(const xAOD::Jet& jet, float rpt) {
              rptHandle(jet) = rpt;

          }
      };


      struct TrkAugHandleHolder {
          SG::ReadDecorHandle<xAOD::JetContainer, std::vector<float>> sumPtTrkHandle;
          SG::ReadDecorHandle<xAOD::JetContainer, std::vector<float>> trkWidthHandle;
          SG::ReadDecorHandle<xAOD::JetContainer, std::vector<int>> numTrkHandle;

          SG::WriteDecorHandle<xAOD::JetContainer, std::vector<float>> DtrkWidthHandle;
          SG::WriteDecorHandle<xAOD::JetContainer, std::vector<int>> DnumTrkHandle;
          SG::WriteDecorHandle<xAOD::JetContainer, std::vector<float>> DrptPerVertexHandle;
          SG::WriteDecorHandle<xAOD::JetContainer, std::vector<float>> rptPerVertexHandle;
          SG::WriteDecorHandle<xAOD::JetContainer, std::vector<float>> trkWidthSortedHandle;
          SG::WriteDecorHandle<xAOD::JetContainer, std::vector<int>> numTrkSortedHandle;

          // Constructor should take ReadDecorHandleKey and WriteDecorHandleKey
          TrkAugHandleHolder(
              const SG::ReadDecorHandleKey<xAOD::JetContainer>& trkWidthKey,
              const SG::ReadDecorHandleKey<xAOD::JetContainer>& sumPtTrkKey,
              const SG::ReadDecorHandleKey<xAOD::JetContainer>& numTrkKey,
              const SG::WriteDecorHandleKey<xAOD::JetContainer>& DtrkWidthKey,
              const SG::WriteDecorHandleKey<xAOD::JetContainer>& DnumTrkKey,
              const SG::WriteDecorHandleKey<xAOD::JetContainer>& DrptPerVertexKey,
              const SG::WriteDecorHandleKey<xAOD::JetContainer>& rptPerVertexKey,
              const SG::WriteDecorHandleKey<xAOD::JetContainer>& TrkWidthSortedKey,
              const SG::WriteDecorHandleKey<xAOD::JetContainer>& NumTrkSortedKey
          ) :
              sumPtTrkHandle(sumPtTrkKey),
              trkWidthHandle(trkWidthKey),
              numTrkHandle(numTrkKey),
              DtrkWidthHandle(DtrkWidthKey),
              DnumTrkHandle(DnumTrkKey),
              DrptPerVertexHandle(DrptPerVertexKey),
              rptPerVertexHandle(rptPerVertexKey),
              trkWidthSortedHandle(TrkWidthSortedKey),
              numTrkSortedHandle(NumTrkSortedKey)
          {}

            TrackMomentStruct getTrackMoments(const xAOD::Jet& jet) {
                return TrackMomentStruct(numTrkHandle(jet), trkWidthHandle(jet), sumPtTrkHandle(jet));
            }

            void decorate(const xAOD::Jet& jet,
                  const std::vector<float>& rptVec, const std::vector<int>& dNumTrkVec,
                  const std::vector<float>& dTrkWidthVec, const std::vector<float>& dRptVec,
                  const std::vector<float>& sortedTrkWidthVec, const std::vector<int>& sortedNumTrkVec) {

                rptPerVertexHandle(jet) = rptVec;
                DnumTrkHandle(jet) = dNumTrkVec;
                DtrkWidthHandle(jet) = dTrkWidthVec;
                DrptPerVertexHandle(jet) = dRptVec;
                trkWidthSortedHandle(jet) = sortedTrkWidthVec;
                numTrkSortedHandle(jet) = sortedNumTrkVec;
            }
      };




    private:

      // Retrieve hard scatter vertex for its index. Return nullptr if one cannot be found
      const xAOD::Vertex *findHSVertex(const xAOD::VertexContainer& vertices) const;

      // Evaluate JVT from Rpt and JVFcorr
      float evaluateJvt(const std::vector<float>& features) const;


      /// Internal members for interpreting jet inputs
      /// and NN configuration
      std::unique_ptr<CachedNNGraph> m_lwnn {nullptr};
      // The Jvt bins and cut map
      NNJvtCutMap m_cutMap;

      // Generically needed for moment tools
      Gaudi::Property<std::string> m_jetContainerName{this,"JetContainer", "", "SG key for the input jet container"};
      Gaudi::Property<bool> m_suppressInputDeps{this, "SuppressInputDependence", false, "Will JVFCorr and SumPtTrk be created in the same algorithm that uses this tool?"};
      Gaudi::Property<bool> m_suppressOutputDeps{this, "SuppressOutputDependence", false, "Ignore creating the output decoration dependency for data flow; for analysis"};

      Gaudi::Property<bool> m_useTrkAugNN{this, "UseTrkAugNN", false, "Flag to select whether to use the Track-Augmented NN configuration"};


      // NN configuration
      Gaudi::Property<std::string> m_TrkAugNNConfigDir{this, "TrkAugNNConfigDir", "JetPileupTag/NNJvt/HLT-2025-02-05", "PathResolver-accessible directory holding config files"};
      Gaudi::Property<std::string> m_TrkAugNNParamFileName{this, "TrkAugNNParamFile", "TrkAugNNJVT.Network.graph.HLT.json", "Name of json file containing network parameters"};
      Gaudi::Property<std::string> m_TrkAugNNCutFileName{this, "TrkAugNNCutFile", "TrkAugNNJVT.Cuts.HLT.json", "Name of json file containing network parameters"};
      Gaudi::Property<std::string> m_NNConfigDir{this,"NNConfigDir", "JetPileupTag/NNJvt/2022-03-22", "PathResolver-accessible directory holding config files"};
      Gaudi::Property<std::string> m_NNParamFileName{this,"NNParamFile", "NNJVT.Network.graph.Offline.Nonprompt_All_MaxWeight.json", "Name of json file containing network parameters"};
      Gaudi::Property<std::string> m_NNCutFileName{this,"NNCutFile", "NNJVT.Cuts.FixedEffPt.Offline.Nonprompt_All_MaxW.json", "Name of json file containing network parameters"};


      // Additional steering 
      Gaudi::Property<float> m_maxpt_for_cut{this,"MaxPtForCut", 60*GeV, "Jet pt above which no cut is applied"};


      // Access to inputs from StoreGate
      SG::ReadHandleKey<xAOD::VertexContainer> m_vertexContainer_key{this, "VertexContainer", "PrimaryVertices", "SG key for input vertex container"};
      SG::ReadDecorHandleKey<xAOD::JetContainer> m_jvfCorrKey{this, "JVFCorrName", "JVFCorr", "SG key for input JVFCorr decoration"};
      SG::ReadDecorHandleKey<xAOD::JetContainer> m_sumPtTrkKey{this, "SumPtTrkName", "SumPtTrkPt500", "SG key for input SumPtTrk decoration"};
      SG::ReadDecorHandleKey<xAOD::JetContainer> m_trkWidthKey{this, "TrackWidthName", "TrackWidthPt1000", "SG key for input TrackWidth decoration"};
      SG::ReadDecorHandleKey<xAOD::JetContainer> m_numTrkKey{this, "NumTrkName", "NumTrkPt1000", "SG key for input NumTrk decoration"};


      SG::WriteDecorHandleKey<xAOD::JetContainer> m_jvtKey{this, "JVTName", "NNJvt", "SG key for output JVT decoration"};
      SG::WriteDecorHandleKey<xAOD::JetContainer> m_rptKey{this, "RpTName", "NNJvtRpt", "SG key for output RpT decoration"};
      SG::WriteDecorHandleKey<xAOD::JetContainer> m_passJvtKey{this, "passJvtName", "NNJvtPass", "SG key for output pass-JVT decoration"};


      //new nnjvt input variables 

      SG::WriteDecorHandleKey<xAOD::JetContainer> m_rptPerVertexKey{this, "rptPerVertexName", "RPtTrkPt500", "SG key for input per vertex RPtTrkPt decoration, vector inputs ordered by sum pt of tracks assigned to jet"};
      SG::WriteDecorHandleKey<xAOD::JetContainer> m_DtrkWidthKey{this, "DTrackWidthName", "DTrackWidthPt1000", "SG key for input DTrackWidth decoration"};
      SG::WriteDecorHandleKey<xAOD::JetContainer> m_DnumTrkKey{this, "DNumTrkName", "DNumTrkPt1000", "SG key for input DNumTrk decoration"};

      SG::WriteDecorHandleKey<xAOD::JetContainer> m_DrptPerVertexKey{this, "DrptPerVertexName", "DRPtTrkPt500", "SG key for input DRPtTrkPt decoration"};


      //sorted track moments, write
      SG::WriteDecorHandleKey<xAOD::JetContainer> m_TrkWidthSortedKey{this, "TrkWidthSortedName", "SumPtTrkOrderedTrackWidthPt1000", "SG key for input TrackWidth decoration, vector inputs ordered by sum pt of tracks assigned to jet"};
      SG::WriteDecorHandleKey<xAOD::JetContainer> m_NumTrkSortedKey{this, "NumTrkSortedName", "SumPtTrkOrderedNumTrkPt1000", "SG key for input NumTrk decoration, vector inputs ordered by sum pt of tracks assigned to jet"};

      // resort ordering of jet moment vectors according to the descending sumtrkpt

      std::vector<size_t> get_resorted_vertex_indices(const std::vector<float>& jet_sumpt_per_vertex, const xAOD::Vertex* HSvertex) const; 

      OrderedTrackMoment get_sorted_track_moments(const TrackMomentStruct& moments, const std::vector<size_t>& resorted_vertex_indices, float jetPt,  float invalidRpt) const; 

      DTrackMomentStruct calculatePerVertexDifferences(const OrderedTrackMoment& sortedTrackMoments, size_t numVertices) const;

  };

}
#endif
