/*
  Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration
*/

#include "JetCalibTools/JMSCalibStep.h"
#include "JetToolHelpers/HistoInputBase.h"

JMSCalibStep::JMSCalibStep(const std::string& name)
  : asg::AsgTool( name ){ }

StatusCode JMSCalibStep::initialize() {

  ATH_MSG_DEBUG("Initializing the JMS Calibration tool");

  ATH_MSG_DEBUG("Reading from " << m_jetInScale << " and writing to " << m_jetOutScale);
  
  ATH_CHECK(m_histTool.retrieve());

  // Determine the maximum value on the z-azis for which the calibration will be applied, this should be eta
  JetHelper::HistoInputBase* histoTool = dynamic_cast<JetHelper::HistoInputBase*>( &(*m_histTool) );
  if(histoTool){
    TH1 *h = &histoTool->getHistogram();
    m_maxEta = h->GetZaxis()->GetBinLowEdge(h->GetNbinsZ()+1);
  }
  else{
    ATH_MSG_ERROR ("Dynamic cast to JetHelper::HistoInputBase failed in JMSCalibStep!");
  }

  return StatusCode::SUCCESS;

}

StatusCode JMSCalibStep::calibrate(xAOD::JetContainer& jets) const {

  ATH_MSG_DEBUG("Calibrating jet mass");
  
  const xAOD::JetAttributeAccessor::AccessorWrapper<xAOD::JetFourMom_t> jmsScaleMomAcc(m_jetOutScale); 

  for(xAOD::Jet* jet: jets){
      
    const xAOD::JetFourMom_t jetStartP4 = jet->getAttribute<xAOD::JetFourMom_t>(m_jetInScale);
    jet->setJetP4(jetStartP4);

    // Get the calibration factor (only for jets if the pT or energy is above threshold and eta within the histogram z-axis range)
    double massFactor = 1.0;
    JetHelper::JetContext jc;
    if(m_varToolX->getValue(*jet,jc) >= m_minValue_JMS){
      if(m_varToolZ->getValue(*jet,jc) <= m_maxEta){
	massFactor = m_histTool->getValue(*jet, jc);
      }
    }

    // Calculate the corrected mass
    double mass_corr = jetStartP4.mass();

    if(massFactor != 0){
      mass_corr = jetStartP4.mass()/massFactor;
    }
    // Protection for very large masses
    if (mass_corr > jet->e()){
      mass_corr = jet->m();
    }

    double pT_corr = jetStartP4.pt();
    // For small-R jet mass calibrations, keep the pT value fixed
    if(!m_pTfixed){
      pT_corr = std::sqrt(jetStartP4.e()*jetStartP4.e()-mass_corr*mass_corr)/std::cosh( jetStartP4.eta() );
    }

    // Set the four-vector to the calibrated values
    xAOD::JetFourMom_t calibP4 = xAOD::JetFourMom_t(pT_corr, jetStartP4.eta(), jetStartP4.phi(), mass_corr);
    jmsScaleMomAcc.setAttribute(*jet, calibP4);
    jet->setJetP4(calibP4);
  }

  return StatusCode::SUCCESS;

}
