///////////////////////// -*- C++ -*- /////////////////////////////

/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// InSituJMSCalibStep.cxx 
// Implementation file for class InSituJMSCalibStep
/////////////////////////////////////////////////////////////////// 

#include "JetCalibTools/InSituJMSCalibStep.h"

InSituJMSCalibStep::InSituJMSCalibStep(const std::string& name)
  : asg::AsgTool( name ){ }

/////////////////////////////////////////////////////////////////// 
// Public methods: 
/////////////////////////////////////////////////////////////////// 

StatusCode InSituJMSCalibStep::initialize() {
  ATH_MSG_DEBUG ("Initializing " << name() );
  ATH_MSG_DEBUG("Reading from " << m_jetInScale << " and writing to " << m_jetOutScale);

  if ( m_isMC )
    ATH_MSG_WARNING("InSituJMSCalibStep::calibrate : Running over MC, will not calibrate unless expert option CalibrateMC is set to true");

  ATH_CHECK(m_histTool_AbsJMS.retrieve());
  
  return StatusCode::SUCCESS;
}


StatusCode InSituJMSCalibStep::calibrate(xAOD::JetContainer& jets) const {

  ATH_MSG_DEBUG("Starting InSitu JMS calibration of jet collection.");

  JetHelper::JetContext jc;

  for(xAOD::Jet* jet : jets){
      const xAOD::JetFourMom_t jetStartP4 = jet->getAttribute<xAOD::JetFourMom_t>(m_jetInScale);

      jet->setJetP4(jetStartP4);

      double calibFactor = 1.0;

      calibFactor = 1/m_histTool_AbsJMS->getValue(*jet, jc);

      xAOD::JetFourMom_t calibP4=jetStartP4*calibFactor;

      // pT doesn't change while applying in situ JMS
      TLorentzVector TLVjet;
      TLVjet.SetPtEtaPhiM( jetStartP4.pt(), jetStartP4.eta(), jetStartP4.phi(), calibP4.M() );
      calibP4.SetPxPyPzE( TLVjet.Px(), TLVjet.Py(), TLVjet.Pz(), TLVjet.E() );

      // Set the output scale
      jet->setAttribute<xAOD::JetFourMom_t>(m_jetOutScale, calibP4);
      jet->setJetP4(calibP4);
    }  
    
  return StatusCode::SUCCESS;
}