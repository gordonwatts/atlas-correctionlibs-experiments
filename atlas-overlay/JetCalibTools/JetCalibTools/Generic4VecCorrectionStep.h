/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#ifndef JETCALIBTOOLS_GENERIC4VECCORRECTIONSTEP_H
#define JETCALIBTOOLS_GENERIC4VECCORRECTIONSTEP_H 1

////////////////////////////////////////////////////////////
// Header file for class Generic4VecCorrection
// Implementation of generic four-vector scaling corrections
// Used e.g. for residual AF3 or pT corrections after GSC
////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include "TAxis.h"

#include "AsgTools/AsgTool.h"
#include "AsgTools/AsgToolMacros.h"
#include "AsgTools/ToolHandle.h"
#include <AsgTools/PropertyWrapper.h>

#include "JetAnalysisInterfaces/IJetCalibStep.h"
#include "JetAnalysisInterfaces/IVarTool.h"

#include "TruthUtils/HepMCHelpers.h"

class Generic4VecCorrectionStep
  : public asg::AsgTool,
    virtual public IJetCalibStep {

  ASG_TOOL_CLASS(Generic4VecCorrectionStep, IJetCalibStep)

public:
  /// Constructor with parameters: 
  Generic4VecCorrectionStep(const std::string& name = "Generic4VecCorrectionStep");

  virtual StatusCode initialize() override;
  virtual StatusCode calibrate(xAOD::JetContainer&) const override;

private:
  Gaudi::Property<std::string> m_jetInScale {this, "InScale", "JetGSCScaleMomentum", "Starting jet scale"};
  Gaudi::Property<std::string> m_jetOutScale {this, "OutScale", "", "Ending jet scale"};

  /// Generic histogram with correction factor (e.g. 2D)
  ToolHandle<JetHelper::IVarTool> m_histTool {this, "histoTool", "", "Generic calibration factor"};

  /// use bin center to avoid interpolation along eta (e.g. for PtResidual correction)
  Gaudi::Property<bool> m_useBinCenter = {this, "useBinCenter", false, "boolean to switch to bin center"};
  /// Variable to be used for bin center
  ToolHandle<JetHelper::IVarTool> m_varTool {this, "varTool", "", "input variable for eta, y, detectorEta"};

  /// Properties for MC2MC correction:

  /// Booleans for MC2MC and to turn on c-jet and b-jet corrections
  Gaudi::Property< bool > m_isMC2MCCorr = {this, "isMC2MCCorr", false, "MC2MC correction?"};
  Gaudi::Property< bool > m_doCjetCorrection = {this, "doCjetCorrection", false, "should c-jets be corrected?"};
  Gaudi::Property< bool > m_doBjetCorrection = {this, "doBjetCorrection", false, "should b-jets be corrected?"};

  /// Which truth label should be used
  Gaudi::Property<std::string> m_pidLabel = {this, "PIDLabel", "PartonTruthLabelID", "Parton truth label"};

  /// 2D histogram containing the calibration factors for light quarks
  ToolHandle<JetHelper::IVarTool> m_hist_q {this, "mc2mcHist_q", "", "2D MC2MC light quark calibration"};
  /// 2D histogram containing the calibration factors for gluons
  ToolHandle<JetHelper::IVarTool> m_hist_g {this, "mc2mcHist_g", "", "2D MC2MC gluon calibration"};
  /// 2D histogram containing the calibration factors for charm quarks
  ToolHandle<JetHelper::IVarTool> m_hist_c {this, "mc2mcHist_c", "", "2D MC2MC charm quark calibration"};
  /// 2D histogram containing the calibration factors for bottom quarks
  ToolHandle<JetHelper::IVarTool> m_hist_b {this, "mc2mcHist_b", "", "2D MC2MC bottom quark calibration"};

  std::map<int, ToolHandle<JetHelper::IVarTool> > m_correctionHists;
  
  /// Needed to avoid interpolation between eta bins
  TAxis m_etaAxis;

}; 

#endif
