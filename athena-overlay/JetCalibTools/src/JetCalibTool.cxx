///////////////////////// -*- C++ -*- /////////////////////////////

/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

// JetCalibTool.cxx 
// Implementation file for class JetCalibTool
/////////////////////////////////////////////////////////////////// 

#include "JetCalibTools/JetCalibTool.h"
#include "PathResolver/PathResolver.h"
#include "AsgDataHandles/ReadDecorHandle.h"

JetCalibTool::JetCalibTool(const std::string& name)
  : asg::AsgTool( name ){ }



/////////////////////////////////////////////////////////////////// 
// Public methods: 
/////////////////////////////////////////////////////////////////// 

StatusCode JetCalibTool::initialize() {
  ATH_MSG_DEBUG ("Initializing " << name() );

  ATH_CHECK( m_calibSteps.retrieve());
  if(!m_smearingTool.empty() ){
    ATH_CHECK( m_smearingTool.retrieve());
  }

  ATH_MSG_DEBUG("Jet calibration sequence :");
  for(const ToolHandle<IJetCalibStep> &t : m_calibSteps){
    ATH_MSG_DEBUG("  - "<< t->name() );
  }
  if(!m_smearingTool.empty() ){
    ATH_MSG_DEBUG("  - "<< m_smearingTool->name() );
  }
  
  return StatusCode::SUCCESS;
}


StatusCode JetCalibTool::calibrate(xAOD::JetContainer& jets) const {

  ATH_MSG_DEBUG("calibrating jet collection.");
  for(const ToolHandle<IJetCalibStep>& cstep: m_calibSteps){
    ATH_CHECK( cstep->calibrate(jets) );
  }
  return StatusCode::SUCCESS;
}


StatusCode JetCalibTool::getNominalResolutionData(const xAOD::Jet& jet, const JetHelper::JetContext& jcontext, double& resolution) const{

  if(m_smearingTool.empty()){
    ATH_MSG_ERROR("No smearing tool configured !");
    return StatusCode::FAILURE;
  }
  return m_smearingTool->getNominalResolutionData(jet, jcontext, resolution);
}

StatusCode JetCalibTool::getNominalResolutionMC(const xAOD::Jet& jet,  const JetHelper::JetContext& jcontext, double& resolution) const{
  if(m_smearingTool.empty()){
    ATH_MSG_ERROR("No smearing tool configured !");
    return StatusCode::FAILURE;
  }
  return m_smearingTool->getNominalResolutionMC(jet, jcontext, resolution);
}

