/*
  Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration
*/

#ifndef JETCALIBTOOLS_JMSCALIBSTEP_H
#define JETCALIBTOOLS_JMSCALIBSTEP_H 1

////////////////////////////////////////////////////////////
// Header file for class JMSCalibStep
// Implementation of the MC-based jet mass scale calibration
// Primarily used for large-R jets
////////////////////////////////////////////////////////////

#include <string>

#include "AsgTools/AsgTool.h"
#include "AsgTools/AsgToolMacros.h"
#include "AsgTools/ToolHandle.h"
#include "AsgTools/PropertyWrapper.h"

#include "JetAnalysisInterfaces/IJetCalibStep.h"
#include "JetAnalysisInterfaces/IVarTool.h"

class JMSCalibStep
  : public asg::AsgTool,
    virtual public IJetCalibStep {

  ASG_TOOL_CLASS(JMSCalibStep, IJetCalibStep)

  public:
  JMSCalibStep(const std::string& name = "JMSCalibStep");

  virtual StatusCode initialize() override;
  virtual StatusCode calibrate(xAOD::JetContainer&) const override;

private:

  Gaudi::Property<std::string> m_jetInScale {this, "InScale", "JetEtaJESScaleMomentum", "Starting jet scale"};
  Gaudi::Property<std::string> m_jetOutScale {this, "OutScale", "JetJMSScaleMomentum", "Ending jet scale"};

  /// Properties
  Gaudi::Property< float > m_minValue_JMS = {this, "MinValueForJMS", 180., "min pT or energy value for calibration [GeV]"};
  Gaudi::Property< bool > m_pTfixed = {this, "pTfixed", false, ""};

  /// 3D histogram containing mass calibration values
  ToolHandle<JetHelper::IVarTool> m_histTool {this, "histoReaderJMS", "HistoInput3D", "3D mass calibration histograms"};
  /// Variable used to enforce minimum pT or energy for calibration
  ToolHandle<JetHelper::IVarTool> m_varToolX {this, "varToolX", "VarTool", "input variable for E or pT threshold cut"};
  /// Variable used to enforce eta threshold
  ToolHandle<JetHelper::IVarTool> m_varToolZ {this, "varToolZ", "VarTool", "input variable for eta cut"};

  float m_maxEta = 0.0;
  
};

#endif

