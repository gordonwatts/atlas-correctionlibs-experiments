///////////////////////// -*- C++ -*- /////////////////////////////

/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// InSituCalibStep.cxx 
// Implementation file for class InSituCalibStep
/////////////////////////////////////////////////////////////////// 

#include "JetCalibTools/InSituCalibStep.h"
#include "PathResolver/PathResolver.h"
#include "AsgDataHandles/ReadDecorHandle.h"

InSituCalibStep::InSituCalibStep(const std::string& name)
  : asg::AsgTool( name ){ }

/////////////////////////////////////////////////////////////////// 
// Public methods: 
/////////////////////////////////////////////////////////////////// 

StatusCode InSituCalibStep::initialize() {
  ATH_MSG_DEBUG ("Initializing " << name() );
  ATH_MSG_DEBUG("Reading from " << m_jetInScale << " and writing to " << m_jetOutScale);
  // Initialise ReadHandle(s)
  ATH_CHECK( m_evtInfoKey.initialize() );

  if ( m_isMC )
    ATH_MSG_WARNING("InSituCalibStep::calibrate : Running over MC, will not calibrate unless expert option CalibrateMC is set to true");

  ATH_CHECK( m_histTool_EtaInter.retrieve() ); 
  ATH_CHECK( m_histTool_Abs.retrieve() ); 
  
  if( m_histTool_Abs.size() != m_histTool_EtaInter.size() )
    return StatusCode::FAILURE;
  
  return StatusCode::SUCCESS;
}


StatusCode InSituCalibStep::calibrate(xAOD::JetContainer& jets) const {

  ATH_MSG_DEBUG("calibrating jet collection.");

  // Retrieve EventInfo object, for time-dependent calibration and isMC flag
  unsigned int rNumber = 0;
  if (retrieveEventInfo(rNumber).isFailure())
    return StatusCode::FAILURE;
  if( (bool)(m_isMC) && !( (bool)(m_CalibrateMC)) ) //no calibration
    return StatusCode::SUCCESS;
  unsigned int runNumber = static_cast<unsigned int>(rNumber+0.5);
  // Pick up the correct time-dependent histogram
  unsigned int periodInd = 9999;
  for(unsigned int i=0; i<m_RunNumBoundaries.size()-1; i++){
    unsigned int firstRun = m_RunNumBoundaries[i] + 1.5;
    unsigned int lastRun = m_RunNumBoundaries[i+1]+0.5;
    if (firstRun<=runNumber && runNumber <= lastRun ){
      periodInd = i;
      break;
    }
  }

  if (periodInd==9999){ // periodInd could not be set
    ATH_MSG_WARNING("No calibration found for run number "<<runNumber);
  }

  if( periodInd >= m_histTool_Abs.size() ) //outside run numbers, return no calibration
    return StatusCode::SUCCESS; 
  
  JetHelper::JetContext jc;
  for (xAOD::Jet* jet : jets){
    const xAOD::JetFourMom_t jetStartP4 = jet->getAttribute<xAOD::JetFourMom_t>(m_jetInScale);
    jet->setJetP4(jetStartP4);

    // Retrieve absolute and relative calibration factors
    const double R_abs = m_histTool_Abs[periodInd]->getValue(*jet,jc);
    double c_rel = m_histTool_EtaInter[periodInd]->getValue(*jet,jc);
    double correction = c_rel/R_abs;

    xAOD::JetFourMom_t calibP4=jet->jetP4();
    calibP4 = calibP4 * correction;
    // Set the output scale
    jet->setAttribute<xAOD::JetFourMom_t>(m_jetOutScale,calibP4);
    jet->setJetP4(calibP4);
  }  
  return StatusCode::SUCCESS;
}

StatusCode InSituCalibStep::retrieveEventInfo(unsigned int &r) const {
  
  const xAOD::EventInfo * eventObj = nullptr;
  static std::atomic<unsigned int> eventInfoWarnings = 0;
  SG::ReadHandle<xAOD::EventInfo> rhEvtInfo(m_evtInfoKey);
  if ( rhEvtInfo.isValid() ) {
    eventObj = rhEvtInfo.cptr();
    r = eventObj->runNumber();
  } else {
    ++eventInfoWarnings;
    if ( eventInfoWarnings < 20 )
      ATH_MSG_ERROR("   InSituCalibStep::calibrate : Failed to retrieve event information.");
    return StatusCode::SUCCESS; //error is recoverable, so return SUCCESS
  }
  return StatusCode::SUCCESS;
}

