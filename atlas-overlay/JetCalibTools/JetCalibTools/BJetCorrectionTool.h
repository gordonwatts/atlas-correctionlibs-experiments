//////////////////////// -*- C++ -*- /////////////////////////////

/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

// BJetCorrectionTool.h 
// Header file for class BJetCorrectionTool
// Simple histogram-based pT-dependent correction
// To be replaced in 2025 with new MVA-based regression
//
// Authors: Mohamed Belfkir <mohamed.belfkir@cern.ch>
//          Thomas Strebler <thomas.strebler@cern.ch>
///////////////////////////////////////////////////////////////////
#ifndef JETCALIBTOOLS_BJETCORRECTIONTOOL_H
#define JETCALIBTOOLS_BJETCORRECTIONTOOL_H 1

#include "AsgTools/AsgTool.h"
#include <AsgTools/PropertyWrapper.h>

#include "JetAnalysisInterfaces/IBJetCorrectionTool.h"

#include "xAODJet/Jet.h"

#include <TFile.h>
#include <TH1F.h>

class BJetCorrectionTool : public asg::AsgTool,
			   virtual public IBJetCorrectionTool {

  ASG_TOOL_CLASS1(BJetCorrectionTool, IBJetCorrectionTool)

public:
  /// Constructor with parameters: 
  BJetCorrectionTool(const std::string& name);

  /// Destructor: 
  ~BJetCorrectionTool() = default;

  virtual StatusCode initialize() override;
  virtual StatusCode applyBJetCorrection(xAOD::Jet& jet, bool isSemiLep) const override;

private:

  //Variables for configuration
  Gaudi::Property<std::string> m_calibFileName{this, "calibFile",
      "CalibArea-01/AntiKt4EMPFlow_PtReco_Correction_GN2v01_85_ttbar.root",
      "ROOT file for reconstructed pt correction"};

  // Clean up automatically
  std::unique_ptr<TH1F> m_Semi_Histo;
  std::unique_ptr<TH1F> m_Had_Histo;
  
};

#endif //> !JETCALIBTOOLS_BJETCORRECTIONTOOL_H
