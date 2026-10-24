/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "JetMomentTools/JetNumConstitTool.h"
#include "AsgDataHandles/WriteDecorHandle.h"

//**********************************************************************

JetNumConstitTool::JetNumConstitTool(const std::string& myname)
  : asg::AsgTool(myname)
{}

//**********************************************************************

StatusCode JetNumConstitTool::initialize(){

  if(m_jetContainerName.empty()){
    ATH_MSG_ERROR("JetNumConstitTool needs to have its input jet container name configured!");
    return StatusCode::FAILURE;
  }

  // Prepend jet container name
  m_numConstitKey = m_jetContainerName + "." + m_numConstitKey.key();
  ATH_CHECK(m_numConstitKey.initialize());

  return StatusCode::SUCCESS;
}

//**********************************************************************

StatusCode JetNumConstitTool::decorate(const xAOD::JetContainer& jets) const {

  SG::WriteDecorHandle<xAOD::JetContainer, int> numConstitHandle(m_numConstitKey);

  for(const xAOD::Jet* jet : jets){
    numConstitHandle(*jet) = jet->numConstituents();
  }

  return StatusCode::SUCCESS;
}

//**********************************************************************
