///////////////////////// -*- C++ -*- /////////////////////////////

/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// JetResponseTool.h 
// Header file for class JetResponseTool
/////////////////////////////////////////////////////////////////// 
#ifndef JETCALIBTOOLS_JETRESPONSETOOL_H
#define JETCALIBTOOLS_JETRESPONSETOOL_H 1

#include <string.h>

#include "AsgTools/AsgTool.h"
#include "AsgTools/AsgToolMacros.h"
#include "AsgTools/PropertyWrapper.h"

#include "JetInterface/IJetDecorator.h"

#include "AsgDataHandles/ReadHandleKey.h"
#include "AsgDataHandles/ReadDecorHandleKey.h"
#include "AsgDataHandles/WriteDecorHandleKey.h"

#include "xAODJet/Jet.h"
#include "xAODJet/JetContainer.h"



class JetResponseTool
  : public asg::AsgTool,
    virtual public IJetDecorator {

  ASG_TOOL_CLASS(JetResponseTool, IJetDecorator)

public:
  /// Constructor with parameters: 
  JetResponseTool(const std::string& name = "JetResponseTool");

  virtual StatusCode initialize() override;
  virtual StatusCode decorate(const xAOD::JetContainer& jets) const override;

private:

  Gaudi::Property<float> m_recoJetMinPt{this, "RecoJetMinPt", 7000, "Minimum reco jet pT in MeV"};
  Gaudi::Property<float> m_truthIsolMaxFrac{this, "TruthIsolMaxFrac", 0.3, "Maximum truth particle pt in isolation cone"};
  Gaudi::Property<float> m_recoIsolMaxFrac{this, "RecoIsolMaxFrac", 0.3, "Maximum reco constituent pt in isolation cone"};

  bool isIsolated(float recoIsolFrac, float truthIsolFrac) const;

  SG::ReadHandleKey<xAOD::JetContainer> m_jetContainerKey{this, "JetContainer", "", "SG key for the input jet container"};
  SG::ReadHandleKey<xAOD::JetContainer> m_truthJetContainerKey{this, "TruthJetContainer", "", "SG key for the truth jet container"}; // For isolation decoration
  SG::ReadDecorHandleKey<xAOD::JetContainer> m_jetMatchedTruthJetKey{this, "JetMatchedTruthJetName", "_MatchedTruthJet", "SG key for the matched truth jet ElementLink attribute"};

  SG::ReadDecorHandleKey<xAOD::JetContainer> m_jetTruthIsolKey{this, "JetTruthIsolName", "IsoFixedCone5Pt", "SG key for the matched truth jet ElementLink attribute"};
  SG::ReadDecorHandleKey<xAOD::JetContainer> m_jetRecoIsolKey{this, "JetRecoIsolName", "IsoFixedCone5PtPUsub", "SG key for the matched truth jet ElementLink attribute"};

  SG::WriteDecorHandleKey<xAOD::JetContainer> m_jetResponseKey{this, "JetResponseName", "JetResponse", "SG key for the JetResponse attribute"};
  SG::WriteDecorHandleKey<xAOD::JetContainer> m_jetIsolatedKey{this, "JetIsolatedName", "JetResponseIsolated", "SG key for the JetResponseIsolated attribute"};

}; 

#endif //> !JETCALIBTOOLS_JETRESPONSETOOL_H
