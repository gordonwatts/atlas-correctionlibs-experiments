//////////////////////// -*- C++ -*- /////////////////////////////

/*
  Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration
*/

// JetCalibrationTool.cxx 
// Implementation file for class BJetCorrectionTool
/////////////////////////////////////////////////////////////////// 

#include "JetCalibTools/BJetCorrectionTool.h"
#include "PathResolver/PathResolver.h"

BJetCorrectionTool::BJetCorrectionTool(const std::string& name) : asg::AsgTool (name) { }

StatusCode BJetCorrectionTool::initialize() {
  std::string filename = PathResolverFindCalibFile("BJetPtCorrection/" + m_calibFileName);
  std::unique_ptr<TFile> calibFile = std::make_unique<TFile>(filename.c_str(), "READ");
  if(!calibFile){
    ANA_MSG_FATAL("Calibration file is not found " << filename );
    return StatusCode::FAILURE;
  }

  m_Semi_Histo.reset(dynamic_cast<TH1F*>(calibFile->Get("Correction_SemiLeptonic_ttbar_mean")));
  if (!m_Semi_Histo) {
    ATH_MSG_FATAL("Cannot get Correction_SemiLeptonic_ttbar_mean from calib file");
    return StatusCode::FAILURE;
  }
  m_Semi_Histo->SetDirectory(nullptr);
  m_Had_Histo.reset(dynamic_cast<TH1F*>(calibFile->Get("Correction_Hadronic_ttbar_mean")));
  if (!m_Had_Histo) {
    ATH_MSG_FATAL("Cannot get Correction_Hadronic_ttbar_mean from calib file");
    return StatusCode::FAILURE;
  }
  m_Had_Histo->SetDirectory(nullptr);
  calibFile->Close();

  return StatusCode::SUCCESS;
}

StatusCode BJetCorrectionTool::applyBJetCorrection(xAOD::Jet& jet, bool isSemiLep) const {
  float calibFactor = (isSemiLep ? m_Semi_Histo : m_Had_Histo)->Interpolate(std::log(jet.pt() * 0.001));
  TLorentzVector j = calibFactor * jet.p4();
  xAOD::JetFourMom_t new_jet(j.Pt(),j.Eta(),j.Phi(),j.M());
  jet.setJetP4(new_jet);
  return StatusCode::SUCCESS;
}
