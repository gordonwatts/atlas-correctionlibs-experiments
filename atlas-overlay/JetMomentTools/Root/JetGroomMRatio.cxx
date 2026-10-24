/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "JetMomentTools/JetGroomMRatio.h"

#include "xAODJet/JetAccessorMap.h"
#include "AsgDataHandles/WriteDecorHandle.h"

//**********************************************************************

JetGroomMRatio::JetGroomMRatio(const std::string& name)
: AsgTool(name) { }

//**********************************************************************

StatusCode JetGroomMRatio::initialize() {
  ATH_MSG_INFO("Initializing JetGroomMRatio " << name());
  
  if(m_jetContainerName.empty()){
    ATH_MSG_ERROR("JetGroomMRatio needs to have its input jet container configured!");
    return StatusCode::FAILURE;
  }
  
  m_groomMRatioKey = m_jetContainerName + "." + m_groomMRatioKey.key();

  ATH_CHECK(m_groomMRatioKey.initialize());

  return StatusCode::SUCCESS;
}

//**********************************************************************

StatusCode JetGroomMRatio::decorate(const xAOD::JetContainer& jets) const {
  ATH_MSG_VERBOSE("Begin decorating jets.");
  const SG::AuxElement::ConstAccessor< ElementLink<xAOD::JetContainer> > parentAcc("Parent");
  const std::string uncalP4Str{"JetConstitScaleMomentum"};
  for(const xAOD::Jet* jet : jets) {
    const xAOD::Jet* parent = *parentAcc(*jet);

    if (parent==nullptr) {
      ATH_MSG_ERROR("No ungroomed parent jet available");
      return StatusCode::FAILURE;
    }

    SG::WriteDecorHandle<xAOD::JetContainer, float> groomMRatioHandle(m_groomMRatioKey);

    // Get jet p4 at constituent scale to compute ratio
    xAOD::JetFourMom_t uncalP4;
    jet->getAttribute<xAOD::JetFourMom_t>(uncalP4Str,uncalP4);

    groomMRatioHandle(*jet) = uncalP4.M()/parent->m();

  }
  
  return StatusCode::SUCCESS;
}
