/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "JetCalibTools/Generic4VecCorrectionStep.h"
#include "JetToolHelpers/HistoInputBase.h"

#include <cmath>

Generic4VecCorrectionStep::Generic4VecCorrectionStep(const std::string& name)
  : asg::AsgTool( name ){ }

StatusCode Generic4VecCorrectionStep::initialize(){

  ATH_MSG_DEBUG("Initializing Generic4VecCorrectionStep tool");
  ATH_MSG_DEBUG("Reading from " << m_jetInScale << " and writing to " << m_jetOutScale);

  if(m_isMC2MCCorr){
    std::vector<int> considered_PIDs = {MC::DQUARK,MC::UQUARK,MC::SQUARK,MC::GLUON};

    ATH_CHECK(m_hist_q.retrieve());
    ATH_CHECK(m_hist_g.retrieve());
    if(m_doCjetCorrection){
      considered_PIDs.push_back(MC::CQUARK);
      ATH_CHECK(m_hist_c.retrieve());
    }
    if(m_doBjetCorrection){
      considered_PIDs.push_back(MC::BQUARK);
      ATH_CHECK(m_hist_b.retrieve());
    }

    for (auto this_PID : considered_PIDs){
      if(this_PID == MC::DQUARK || this_PID == MC::UQUARK || this_PID == MC::SQUARK){
	m_correctionHists.try_emplace(this_PID, m_hist_q);
      } else if(this_PID == MC::CQUARK){
	m_correctionHists.try_emplace(this_PID, m_hist_c);
      } else if(this_PID == MC::BQUARK){
	m_correctionHists.try_emplace(this_PID, m_hist_b);
      } else if(this_PID == MC::GLUON){
	m_correctionHists.try_emplace(this_PID, m_hist_g);
      }
    }
  }
  else{
    ATH_CHECK(m_histTool.retrieve());

    if(m_useBinCenter){
      JetHelper::HistoInputBase* histoTool = dynamic_cast<JetHelper::HistoInputBase*>( &(*m_histTool) );
      if(histoTool){
	TH1 *h = &histoTool->getHistogram();
	m_etaAxis = *(h->GetYaxis());
      }
    }
  }
  
  return StatusCode::SUCCESS;
  
}

StatusCode Generic4VecCorrectionStep::calibrate(xAOD::JetContainer& jets) const {

  ATH_MSG_DEBUG("Applying generic four-vector correction");

  const xAOD::JetAttributeAccessor::AccessorWrapper<xAOD::JetFourMom_t> scaleMomAcc(m_jetOutScale); 

  for(xAOD::Jet* jet: jets){

    const xAOD::JetFourMom_t jetStartP4 = jet->getAttribute<xAOD::JetFourMom_t>(m_jetInScale);
    jet->setJetP4(jetStartP4);

    JetHelper::JetContext jc;
    // In some cases, we want to avoid the interpolation along eta
    // Therefore, we set the eta value to the histogram bin center before reading the histograms
    if(m_useBinCenter){
      // Using a varTool lets us configure eta (eta, y, detectorEta) via yaml file
      int eta_bin = m_etaAxis.FindBin(m_varTool->getValue(*jet, jc));
      jc.setValue("binCenterEta", m_etaAxis.GetBinCenter(eta_bin));
    }
    
    // Get the correction factor:
    double correctionFactor = 1.0;
    
    // MC2MC correction depending on the parton truth label
    if(m_isMC2MCCorr){
      // Retrieve the parton truth label
      static const SG::ConstAccessor<int> PartonTruthLabelIDAcc(m_pidLabel); 
      if(!PartonTruthLabelIDAcc.isAvailable(*jet)){
	ATH_MSG_ERROR("The parton truth label could not be retrieved");
	return StatusCode::FAILURE;
      }
      int label = std::abs(PartonTruthLabelIDAcc(*jet));

      // If this parton truth label ID is in the map, get the correction factor
      auto correction_from_map = m_correctionHists.find(label);
      if (correction_from_map != m_correctionHists.end()){
	auto h_corr_map = correction_from_map->second;
	correctionFactor = h_corr_map->getValue(*jet, jc);
      }
    }
    // other generic 2D corrections (e.g. AF3, PtResidual)
    else{
      correctionFactor = m_histTool->getValue(*jet, jc);
    }
    
    xAOD::JetFourMom_t calibP4 = jet->jetP4() * correctionFactor;
    scaleMomAcc.setAttribute(*jet,calibP4);
    jet->setJetP4(calibP4);   
  }
  
  return StatusCode::SUCCESS;

}
