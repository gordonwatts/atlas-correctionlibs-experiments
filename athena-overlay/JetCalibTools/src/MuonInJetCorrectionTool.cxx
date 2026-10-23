//////////////////////// -*- C++ -*- /////////////////////////////

/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

// MuonInJetCorrectionTool.cxx 
// Implementation file for class MuonInJetCorrectionTool
/////////////////////////////////////////////////////////////////// 

#include "JetCalibTools/MuonInJetCorrectionTool.h"

MuonInJetCorrectionTool::MuonInJetCorrectionTool(const std::string& name)
  : asg::AsgTool( name ){ }

StatusCode MuonInJetCorrectionTool::applyMuonInJetCorrection
(xAOD::Jet& jet, const std::vector< const xAOD::Muon* >& muons, int& nmuons) const {
  const xAOD::Muon* muon_in_jet = getMuonInJet(jet, muons, nmuons);

  if(nmuons>0){
    float eLoss = 0.0;
    muon_in_jet->parameter(eLoss,xAOD::Muon::ParamDef::EnergyLoss);
    TLorentzVector mu_tlv = muon_in_jet->p4();
    // Loss with same direction as muon + mass = 0
    TLorentzVector Loss;
    Loss.SetVectM(eLoss * (mu_tlv.Vect().Unit()), 0.);
    TLorentzVector j = jet.p4() - Loss + mu_tlv;
    xAOD::JetFourMom_t new_jet(j.Pt(), jet.eta(), jet.phi(), j.M());
    jet.setJetP4(new_jet);
  }

  return StatusCode::SUCCESS; 
}

const xAOD::Muon* MuonInJetCorrectionTool::getMuonInJet
(const xAOD::Jet& jet, const std::vector< const xAOD::Muon* >& muons, int& nmuons) const
{
  const xAOD::Muon* muon_in_jet = nullptr;

  for(const auto& muon : muons){
    double dR = jet.p4().DeltaR(muon->p4());
    double dR_max = m_Jet_Muon_dR;
    if(m_doVR){
      if(m_doLargeR) dR_max = std::min(1.0, 0.04 + 200000./muon->pt());
      else dR_max = std::min(0.4, 0.04 + 10000./muon->pt());
    }
    if(dR > dR_max) continue;

    nmuons++;
    if(!muon_in_jet || muon->pt() > muon_in_jet->pt()) muon_in_jet = muon;
  }

  return muon_in_jet;
}
