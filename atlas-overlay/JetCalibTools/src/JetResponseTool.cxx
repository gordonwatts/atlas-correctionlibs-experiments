///////////////////////// -*- C++ -*- /////////////////////////////

/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// JetResponseTool.cxx 
// Implementation file for class JetResponseTool
/////////////////////////////////////////////////////////////////// 

#include <iomanip>

#include "JetCalibTools/JetResponseTool.h"
#include "AsgDataHandles/ReadHandle.h"
#include "AsgDataHandles/WriteDecorHandle.h"
#include "AsgDataHandles/ReadDecorHandle.h"
#include "AthContainers/ConstDataVector.h"
#include "FourMomUtils/xAODP4Helpers.h"

JetResponseTool::JetResponseTool(const std::string& name)
  : asg::AsgTool( name ){ }



/////////////////////////////////////////////////////////////////// 
// Public methods: 
/////////////////////////////////////////////////////////////////// 

StatusCode JetResponseTool::initialize() {
  ATH_MSG_DEBUG ("Initializing " << name() );

  // Data read from reco jets
  ATH_CHECK(m_jetContainerKey.initialize());

  m_jetMatchedTruthJetKey = SG::ReadDecorHandleKey<xAOD::JetContainer>(m_jetContainerKey, m_jetMatchedTruthJetKey.key());
  ATH_CHECK(m_jetMatchedTruthJetKey.initialize());

  m_jetRecoIsolKey = SG::ReadDecorHandleKey<xAOD::JetContainer>(m_jetContainerKey, m_jetRecoIsolKey.key());
  ATH_CHECK(m_jetRecoIsolKey.initialize());

  // Data read from truth jets
  ATH_CHECK(m_truthJetContainerKey.initialize()); // Not used directly but it has to be there

  m_jetTruthIsolKey = SG::ReadDecorHandleKey<xAOD::JetContainer>(m_truthJetContainerKey, m_jetTruthIsolKey.key());
  ATH_CHECK(m_jetTruthIsolKey.initialize());

  // Decorations to write
  m_jetResponseKey = SG::WriteDecorHandleKey<xAOD::JetContainer>(m_jetContainerKey, m_jetResponseKey.key());
  m_jetIsolatedKey = SG::WriteDecorHandleKey<xAOD::JetContainer>(m_jetContainerKey, m_jetIsolatedKey.key());

  ATH_CHECK(m_jetResponseKey.initialize());
  ATH_CHECK(m_jetIsolatedKey.initialize());
  
  return StatusCode::SUCCESS;
}


StatusCode JetResponseTool::decorate(const xAOD::JetContainer& jets) const {
  SG::ReadDecorHandle<xAOD::JetContainer, ElementLink<xAOD::JetContainer> > jetMatchedTruthJetHandle(m_jetMatchedTruthJetKey);
  SG::ReadDecorHandle<xAOD::JetContainer, float> jetRecoIsolHandle(m_jetRecoIsolKey);
  SG::ReadDecorHandle<xAOD::JetContainer, float> jetTruthIsolHandle(m_jetTruthIsolKey);
  SG::WriteDecorHandle<xAOD::JetContainer, float> jetResponseHandle(m_jetResponseKey);
  SG::WriteDecorHandle<xAOD::JetContainer, char> jetIsolatedHandle(m_jetIsolatedKey);

  ConstDataVector<xAOD::JetContainer> sel_jets(SG::VIEW_ELEMENTS);
  for(const xAOD::Jet* jet : jets) {
    jetResponseHandle(*jet) = -1.; // Invalid initialisation
    jetIsolatedHandle(*jet) = false;
    if(jet->pt() > m_recoJetMinPt) {
        sel_jets.push_back(jet);
    }
  }

  ATH_MSG_DEBUG("Selected " << sel_jets.size() << " reco jets above " << m_recoJetMinPt.value() << " MeV");

  for(const xAOD::Jet* jet : sel_jets) {
    if(jetMatchedTruthJetHandle(*jet).isValid()) {
      const xAOD::Jet* matched_truthjet = *jetMatchedTruthJetHandle(*jet);
      float Eresponse = jet->e() / matched_truthjet->e();
      ATH_MSG_VERBOSE("  Jet energy response : " << std::setprecision(3) << Eresponse);
      jetResponseHandle(*jet) = Eresponse;
    }

    bool isIsol = isIsolated(jetRecoIsolHandle(*jet),jetTruthIsolHandle(*jet));
    jetIsolatedHandle(*jet) = isIsol;
    ATH_MSG_VERBOSE("  Jet " << jet->index() << " with pt " << std::setprecision(3) << jet->pt() << " isolated? " << isIsol);
    ATH_MSG_VERBOSE("  Isolation fractions: Truth " << std::setprecision(3) << jetTruthIsolHandle(*jet) << ", Reco " << jetRecoIsolHandle(*jet));
  }

  return StatusCode::SUCCESS;
}


bool JetResponseTool::isIsolated(float recoIsolFrac, float truthIsolFrac) const {
  return (recoIsolFrac < m_recoIsolMaxFrac) && (truthIsolFrac < m_truthIsolMaxFrac);
}
