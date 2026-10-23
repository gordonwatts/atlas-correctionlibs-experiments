///////////////////////// -*- C++ -*- /////////////////////////////

/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// InSituJMSCalibStep.h 
// Header file for class InSituJMSCalibStep
/////////////////////////////////////////////////////////////////// 

#ifndef JETCALIBTOOLS_INSITUJMSCALIBSTEP_H
#define JETCALIBTOOLS_INSITUJMSCALIBSTEP_H 1

#include <string.h>

#include <TString.h>
#include <TEnv.h>

#include "AsgTools/AsgTool.h"
#include "AsgTools/AsgToolMacros.h"
#include "AsgTools/ToolHandle.h"
#include "AsgTools/PropertyWrapper.h"
#include "AsgDataHandles/ReadDecorHandleKey.h"

#include "JetAnalysisInterfaces/IJetCalibTool.h"
#include "JetAnalysisInterfaces/IJetCalibStep.h"
#include "JetAnalysisInterfaces/IVarTool.h"
#include "JetToolHelpers/InputVariable.h"
#include "JetToolHelpers/HistoInputBase.h"

class InSituJMSCalibStep
  : public asg::AsgTool,
    virtual public IJetCalibStep {
   ASG_TOOL_CLASS(InSituJMSCalibStep, IJetCalibStep)

public:
  /// Constructor with parameters: 
  InSituJMSCalibStep(const std::string& name = "InSituJMSCalibStep");

  virtual StatusCode initialize() override;
  virtual StatusCode calibrate(xAOD::JetContainer&) const override;


private:
 
  Gaudi::Property<bool> m_CalibrateMC {this, "CalibrateMC", false, "force Insitu JMS step for MC sample"};
  Gaudi::Property<bool> m_isMC {this, "isMC", false, "isMC"};
 
  Gaudi::Property<std::string> m_jetInScale {this, "InScale", "JetInsituScaleMomentum", "Starting jet scale"};
  Gaudi::Property<std::string> m_jetOutScale {this, "OutScale", "JetInsituScaleMomentum", "Ending jet scale"};

  ToolHandle<JetHelper::IVarTool> m_histTool_AbsJMS {this, "HistoReaderAbsJMS", "HistoInput2D", "Instance of HistoInput2D for reading histogram"};
};
#endif
