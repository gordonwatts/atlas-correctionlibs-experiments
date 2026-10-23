/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include <TEnv.h>
#include "JetCalibTools/IJetCalibrationTool.h"

#include "JetCalibTools/PileupAreaCalibStep.h"

#include "PathResolver/PathResolver.h"
#include "xAODJet/JetAccessors.h"
#include "AsgDataHandles/ReadDecorHandle.h"

PileupAreaCalibStep::PileupAreaCalibStep(const std::string& name)
  : asg::AsgTool( name ) { }




StatusCode PileupAreaCalibStep::initialize() {

  ATH_MSG_INFO("Initializing pileup area correction.");

  ATH_MSG_DEBUG("Reading from " << m_jetInScale << " and writing to " << m_jetOutScale);

  ATH_CHECK( m_rhoKey.initialize() );
  return StatusCode::SUCCESS;
}

StatusCode PileupAreaCalibStep::calibrate(xAOD::JetContainer& jetCont) const {

  SG::ReadHandle<xAOD::EventShape> rhRhoKey(m_rhoKey);
  double rho=0;
  if(!rhRhoKey.isValid()){
    ATH_MSG_FATAL("Could not retrieve xAOD::EventShape : "<< m_rhoKey.key());
    return StatusCode::FAILURE;
  }
  const xAOD::EventShape * eventShape = rhRhoKey.cptr();
  
  if ( !eventShape->getDensity( xAOD::EventShape::Density, rho ) ) {
    ATH_MSG_FATAL("Could not retrieve xAOD::EventShape::Density from xAOD::EventShape "<< m_rhoKey.key());
    return StatusCode::FAILURE;  
  }

  ATH_MSG_DEBUG("  Rho = " << 0.001*rho << " GeV");

  const xAOD::JetAttributeAccessor::AccessorWrapper<xAOD::JetFourMom_t> areaAcc("ActiveArea4vec");  
  const xAOD::JetAttributeAccessor::AccessorWrapper<xAOD::JetFourMom_t> outScaleMomAcc(m_jetOutScale);  

  for(xAOD::Jet *jet : jetCont){

    const xAOD::JetFourMom_t jetStartP4 = jet->getAttribute<xAOD::JetFourMom_t>(m_jetInScale);
    jet->setJetP4(jetStartP4);

    xAOD::JetFourMom_t jetareaP4 = areaAcc.getAttribute(*jet);
    ATH_MSG_VERBOSE("    Area = " << jetareaP4);

    xAOD::JetFourMom_t calibP4;
    
    if(m_useFull4vectorArea){
      calibP4 = jetStartP4 - rho*jetareaP4;
    } else {
      ATH_MSG_VERBOSE("  Applying postive-only area-subtraction calibration to jet " << jet->index() << " with pT = " << 0.001*jet->pt() << " GeV");
      const double pT_det = jetStartP4.pt();      
      const double E_det = jetStartP4.e();      
      //Set the jet pT to 10 MeV if the pT or energy is negative after the jet area correction
      const double area_SF = (pT_det-rho*jetareaP4.pt()<=0 || E_det-rho*jetareaP4.e()<=0) ? 10/pT_det : (pT_det-rho*jetareaP4.pt())/pT_det;
      calibP4 = jetStartP4*area_SF;      
    }

    //Transfer calibrated jet properties to the Jet object
    outScaleMomAcc.setAttribute(*jet, calibP4 );
    jet->setJetP4( calibP4 );    
  } 
  return StatusCode::SUCCESS;
}

