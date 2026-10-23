///////////////////////// -*- C++ -*- /////////////////////////////

/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// InSituCalibStep.h 
// Header file for class InSituCalibStep
/////////////////////////////////////////////////////////////////// 
#ifndef JETCALIBTOOLS_INSITUCALIBSTEP_H
#define JETCALIBTOOLS_INSITUCALIBSTEP_H 1

#include <string.h>

#include <TString.h>
#include <TEnv.h>

#include "AsgTools/AsgTool.h"
#include "AsgTools/AsgToolMacros.h"
#include "AsgTools/ToolHandle.h"
#include "AsgTools/PropertyWrapper.h"
#include "AsgDataHandles/ReadDecorHandleKey.h"

#include "xAODEventInfo/EventInfo.h"

#include "JetAnalysisInterfaces/IJetCalibTool.h"
#include "JetAnalysisInterfaces/IJetCalibStep.h"
#include "JetAnalysisInterfaces/IVarTool.h"
#include "JetToolHelpers/InputVariable.h"
#include "JetToolHelpers/HistoInputBase.h"

class InSituCalibStep
  : public asg::AsgTool,
    virtual public IJetCalibStep {
   ASG_TOOL_CLASS(InSituCalibStep, IJetCalibStep)

public:
  /// Constructor with parameters: 
  InSituCalibStep(const std::string& name = "InSituCalibStep");

  virtual StatusCode initialize() override;
  virtual StatusCode calibrate(xAOD::JetContainer&) const override;


private:
 
  Gaudi::Property<bool> m_CalibrateMC {this, "CalibrateMC", false, "force Insitu step for MC sample"};
  Gaudi::Property<bool> m_isMC {this, "isMC", false, "isMC"};
 
  Gaudi::Property<std::string> m_jetInScale {this, "InScale", "JetGSCScaleMomentum", "Starting jet scale"};
  Gaudi::Property<std::string> m_jetOutScale {this, "OutScale", "JetInsituScaleMomentum", "Ending jet scale"};

  // Relative calibration (derived with eta intercalibration)
  ToolHandleArray<JetHelper::IVarTool> m_histTool_EtaInter{this, "HistoReaderEtaInter", {}, "Instance of HistoInput2D for reading histogram"};
  // Absolute calibration (derived with |eta| < 0.8)
  ToolHandleArray<JetHelper::IVarTool> m_histTool_Abs{this, "HistoReaderAbs", {}, "Instance of HistoInput1D for reading histogram"};
  // vector of run numbers
  Gaudi::Property<std::vector<unsigned int> > m_RunNumBoundaries{this, "RunNumbers", {} ,""};
  //ReadHandleKey for event info
  SG::ReadHandleKey<xAOD::EventInfo> m_evtInfoKey{this, "EventInfoKey", "EventInfo"};
  // Method to get runNumber
  StatusCode retrieveEventInfo(unsigned int &r) const;


};
#endif
