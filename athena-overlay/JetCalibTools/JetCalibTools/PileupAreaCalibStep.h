/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

#ifndef JETCALIBTOOLS_JETPILEUPAREACALIBSTEP_H
#define JETCALIBTOOLS_JETPILEUPAREACALIBSTEP_H 1

/* Implementation of JetAreaSubtraction class
 * This class will apply the jet area pile up correction
 *
 * Date: Jan  2024
 */

#include "AsgTools/AsgTool.h"
#include "AsgTools/PropertyWrapper.h"
#include "JetAnalysisInterfaces/IJetCalibStep.h"
#include "xAODEventShape/EventShape.h"
#include "AsgDataHandles/ReadHandleKey.h"


namespace PUCorrection {
  struct PU3DCorrectionHelper;
}

class PileupAreaCalibStep   : public asg::AsgTool,
			      virtual public IJetCalibStep
{

  ASG_TOOL_CLASS(PileupAreaCalibStep, IJetCalibStep) 

 public:
  PileupAreaCalibStep(const std::string& name="PileupAreaCalibStep");

  virtual StatusCode initialize() override;
  virtual StatusCode calibrate(xAOD::JetContainer& jetCont) const override;
 
 private:
  SG::ReadHandleKey<xAOD::EventShape> m_rhoKey{this, "RhoKey", "auto"};

  Gaudi::Property<bool> m_useFull4vectorArea{this, "UseFull4vecArea", false, "doc"};
  Gaudi::Property<bool> m_doOrigin{this, "DoOrigin", false, "doc"};

  Gaudi::Property<std::string> m_jetInScale {this, "InScale", "JetConstitScaleMomentum", "Starting jet scale" };
  Gaudi::Property<std::string> m_jetOutScale {this, "OutScale", "JetAreaSubtractScaleMomentum", "Ending jet scale" };
 
};

#endif
