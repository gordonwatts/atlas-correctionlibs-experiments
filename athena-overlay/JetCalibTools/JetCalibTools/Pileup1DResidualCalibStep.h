/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#ifndef JETCALIBTOOLS_JETPILEUP1DRESIDUALCALIBSTEP_H
#define JETCALIBTOOLS_JETPILEUP1DRESIDUALCALIBSTEP_H 1

/* Pileup1DResidualCalibStep performs the 1 step of the jet calibration
 *
 *  - area subtraction in the form  pT_corr = pT - rho xpT_area 
 *  - residual correction (1D version)
 *
 * Date: Jan  2024
 */

#include "xAODTracking/VertexContainer.h"
#include "xAODEventShape/EventShape.h"
#include "xAODEventInfo/EventInfo.h"

#include "AsgTools/AsgTool.h"
#include "AsgTools/ToolHandle.h"
#include "AsgDataHandles/ReadHandleKey.h"
#include "AsgDataHandles/ReadDecorHandleKey.h"
#include "AsgTools/PropertyWrapper.h"
#include "JetAnalysisInterfaces/IJetCalibStep.h"


class NPVBeamspotCorrection;
class TAxis;

namespace PUCorrection {
  struct PU3DCorrectionHelper;
}

class Pileup1DResidualCalibStep   : public asg::AsgTool,
				    virtual public IJetCalibStep
{

  ASG_TOOL_CLASS(Pileup1DResidualCalibStep, IJetCalibStep) 

 public:
  Pileup1DResidualCalibStep(const std::string& name="Pileup1DResidualCalibStep");

  virtual StatusCode initialize() override;

  // Apply calibration to jet inside given JetContainer (IJetCalibStep interface)
  virtual StatusCode calibrate(xAOD::JetContainer& jetCont) const override;

  // residual calculation (made public for debuging ?)
  double getResidualOffset ( double abseta, double mu, double NPV, int nJet, bool MuOnly, bool NOnly ) const;
  
 private:

  // Internl helper function 
  double getResidualOffsetET(double abseta, double mu, double NPV, int nJet, bool MuOnly, bool NOnly,
                             const std::vector<double>& OffsetMu,
                             const std::vector<double>& OffsetNPV,
                             const std::vector<double>& OffsetNjet,
                             const TAxis *OffsetBins) const;

  
  double getNPVBeamspotCorrection(double NPV) const;



  
  Gaudi::Property<bool> m_isData{this, "IsData", false, ""};
  Gaudi::Property<bool> m_doJetArea{this, "DoJetArea", false, "doc"};

  SG::ReadDecorHandleKey<xAOD::EventInfo> m_muKey {this, "averageInteractionsPerCrossingKey",
      "EventInfo.averageInteractionsPerCrossing","Decoration for Average Interaction Per Crossing"};

  
  SG::ReadHandleKey<xAOD::EventShape> m_rhoKey{this, "RhoKey", "auto"};
  SG::ReadHandleKey<xAOD::VertexContainer> m_pvKey{this, "PrimaryVerticesContainerName", "PrimaryVertices"};

  
  Gaudi::Property<bool> m_doMuOnly{this, "ApplyOnlyMuResidual", false, "doc"};   // only used in ResidualOffsetCorrection
  Gaudi::Property<bool> m_doNPVOnly{this, "ApplyOnlyNPVResidual", false, "doc"}; // only used in ResidualOffsetCorrection
  Gaudi::Property<bool> m_doNJetOnly{this, "ApplyOnlyNJetResidual", false, "doc"}; // only used in ResidualOffsetCorrection

  Gaudi::Property<float> m_NPV_ref{this , "DefaultNPVRef", -99., ""};
  Gaudi::Property<float> m_mu_ref{this , "DefaultMuRef", -99., ""};
  Gaudi::Property<float> m_nJet_ref{this , "DefaultNjetRef", -99., ""};
  Gaudi::Property<float> m_muSF{this , "MuScaleFactor", 1., ""};

  SG::ReadHandleKey<xAOD::JetContainer> m_nJetContainerKey {this, "nJetContainerName",
							    "","a jet container such as HLT_xAOD__JetContainer_a4tcemsubjesISFS"};

  Gaudi::Property<bool> m_useNjet{this , "UseNjet", false, ""};
  Gaudi::Property<int> m_njetThreshold{this , "nJetThreshold", 20, ""};

  
  Gaudi::Property<std::string> m_calibFile {this, "CalibFile", "none", "residual offset calib file"};

  Gaudi::Property<std::vector<double> > m_offsetEtaBins{this, "AbsEtaBins", {} ,""};  
  Gaudi::Property<std::vector<double> > m_resOffsetMu{this, "MuTerm", {} ,""};  
  Gaudi::Property<std::vector<double> > m_resOffsetNjet{this, "nJetTerm", {} ,""};  
  Gaudi::Property<std::vector<double> > m_resOffsetNPV{this, "NPVTerm", {} ,""};  
  TAxis* m_resOffsetBins =nullptr;
  
  
  Gaudi::Property<bool> m_applyNPVBeamspotCorrection{this, "ApplyNPVBeamspotCorrection", false, "doc"}; // only used in ResidualOffsetCorrection  
  NPVBeamspotCorrection * m_npvBeamspotCorr{};

  Gaudi::Property<bool> m_doSequentialResidual{this, "DoSequentialResidual", false, "doc"};

  Gaudi::Property<bool> m_doOnlyResidual {this, "OnlyResidual", false, ""};

  Gaudi::Property<std::string> m_jetInScale {this, "InScale", "JetConstitScaleMomentum", "Starting jet scale" };
  Gaudi::Property<std::string> m_jetOutScale {this, "OutScale", "JetPileupScaleMomentum", "Ending jet scale" };
  
};

#endif
