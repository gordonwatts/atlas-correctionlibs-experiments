///////////////////////// -*- C++ -*- /////////////////////////////

/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

// JetCalibTool.h 
// Header file for class JetCalibTool
/////////////////////////////////////////////////////////////////// 
#ifndef JETCALIBTOOLS_JETCALIBTOOL_H
#define JETCALIBTOOLS_JETCALIBTOOL_H 1

#include <string.h>

#include <TString.h>
#include <TEnv.h>

#include "AsgTools/AsgTool.h"
#include "AsgTools/AsgToolMacros.h"
#include "AsgTools/ToolHandle.h"

#include "xAODEventInfo/EventInfo.h"

#include "JetAnalysisInterfaces/IJetCalibTool.h"
#include "JetAnalysisInterfaces/IJetCalibStep.h"


class JetCalibTool
  : public asg::AsgTool,
    virtual public IJetCalibTool {

  ASG_TOOL_CLASS2(JetCalibTool, IJetCalibTool, IJetModifier)

public:
  /// Constructor with parameters: 
  JetCalibTool(const std::string& name = "JetCalibTool");

  virtual StatusCode initialize() override;
  virtual StatusCode calibrate(xAOD::JetContainer&) const override;

  // Get the nominal resolution
  virtual StatusCode getNominalResolutionData(const xAOD::Jet& jet, const JetHelper::JetContext&, double& resolution) const override;
  virtual StatusCode getNominalResolutionMC(  const xAOD::Jet& jet, const JetHelper::JetContext&, double& resolution) const override;
  
private:


private:

  ToolHandleArray<IJetCalibStep> m_calibSteps {this , "CalibSteps", {}, "calibration steps as IJetCalibStep" };
  
  ToolHandle<IJetCalibStep> m_smearingTool {this , "SmearingTool", {}, "smearing tool as a IJetCalibStep" };
  
}; 

#endif //> !JETCALIBTOOLS_APPLYJETCALIBRATION_H
