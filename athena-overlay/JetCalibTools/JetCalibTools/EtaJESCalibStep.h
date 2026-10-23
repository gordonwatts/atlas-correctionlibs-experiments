///////////////////////// -*- C++ -*- /////////////////////////////

/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// EtaJESCalibStep.h 
// Header file for class EtaJESCalibStep
// Author: Max Swiatlowski <mswiatlo@cern.ch>
/////////////////////////////////////////////////////////////////// 
#ifndef JETCALIBTOOLS_ETAJESCALIBSTEP_H
#define JETCALIBTOOLS_ETAJESCALIBSTEP_H 1

#include <string.h>

#include <TString.h>
#include <TEnv.h>

#include "AsgTools/AsgTool.h"
#include "AsgTools/AsgToolMacros.h"
#include "AsgTools/ToolHandle.h"
#include "AsgTools/PropertyWrapper.h"

#include "xAODEventInfo/EventInfo.h"

#include "JetAnalysisInterfaces/IJetCalibTool.h"
#include "JetAnalysisInterfaces/IJetCalibStep.h"

#include "JetAnalysisInterfaces/IVarTool.h"

class EtaJESCalibStep
  : public asg::AsgTool,
    virtual public IJetCalibStep {

  ASG_TOOL_CLASS(EtaJESCalibStep, IJetCalibStep)

  public:
  EtaJESCalibStep(const std::string& name = "EtaJESCalibStep");

  virtual StatusCode initialize() override;
  virtual StatusCode calibrate(xAOD::JetContainer&) const override;

private:
  // support functions to extract information from text files
  double getLogPolN(const double *factors, double x) const;
  double getLogPolNSlope(const double *factors, double x) const ;
  int getEtaBin(double eta_det) const; 
  bool readMCJESFromText() ;
  bool readMCJESFromHists() ;
  void loadSplineHists(const std::string & fileName, const std::string &etajes_name) ;  

  /// return MCJES calibration factor
  double getJES(const double X, const double Y=0, const double Emax=-1) const;
  /// return Eta correction
  double getEtaCorr( double X,  double Y=0) const;
  /// return Emax
  double getEmaxJES(const double Y) const;
  /// deal with low pt jets
  double getLowPtJES(double E_uncorr, double eta_det) const ;

  double getSplineCorr(const int etaBin, double E) const;
  double getSplineSlope(const int ieta, const double minE) const;
  
  Gaudi::Property<std::string> m_jetInScale {this, "InScale", "JetPileupScaleMomentum", "Starting jet scale"};
  Gaudi::Property<std::string> m_jetOutScale {this, "OutScale", "JetEtaJESScaleMomentum", "Ending jet scale"};

  /// name of the text file
  Gaudi::Property< std::string > m_constantFileName { this, "CalibConstantFile", "", "text file containing constants" };
  /// jet collection to be calibrated
  Gaudi::Property< std::string > m_jetAlgo { this, "JetAlgo", "AntiKt4EMPFlow", "jet collection" };
  Gaudi::Property< bool >  m_freezeJESatHighE = {this, "FreezeJEScorrectionatHighE",false, " freeze at high e"};
  Gaudi::Property< int > m_lowPtExtrap = {this, "LowPtJESExtrapolationMethod", 0, " low pt etrap"};
  Gaudi::Property< float > m_lowPtMinR = {this, "LowPtJESExtrapolationMinimumResponse", 0.25, " low pt etrap min"};

  Gaudi::Property< float >  m_minPt_JES = {this, "MinPtForETAJES",15, "min pT"};
  /// Allowing to use different minPt_JES depending on eta (only for extrapolation methon 1)
  Gaudi::Property< bool > m_useSecondaryminPt_JES = {this, "UseSecondaryMinPtForETAJES", false, "min pT dependent on eta?"};
  /// Starting eta for secondary minPt_JES (Default |eta|>=1.9) (Used only if UseSecondaryMinPtForETAJES is true)
  Gaudi::Property< double > m_etaSecondaryminPt_JES = {this, "EtaSecondaryMinPtForETAJES", 1.9, "eta value for second min pT cut"};
  /// SecondaryminPt_JES (Default 7 GeV) (Used only if UseSecondaryMinPtForETAJES is true)
  Gaudi::Property< float > m_secondaryminPt_JES = {this, "SecondaryMinPtForETAJES", 7.0, "pt value for second min pT cut"};

  Gaudi::Property< float > m_minPt_EtaCorr = {this, "MinPtForEtaCorr" ,8. , ""};
  Gaudi::Property< float > m_maxE_EtaCorr = {this, "MaxEForEtaCorr" ,2500. , ""};
  
  Gaudi::Property< std::string > m_histoFileName { this, "HistoFile", "none", "root file containing histos for spline calib" };
  Gaudi::Property< bool >  m_useSpline = {this, "UseSpline",false, " use spline"};

  /// Eta bins if other than 0.1 steps from -4.5 to 4.5
  Gaudi::Property<std::vector<double> > m_etaBins {this, "EtaBins", {} ,""}; 
  
  ToolHandle<JetHelper::IVarTool> m_vartoolE{this, "VarToolE", "VarTool", "InputVariable instance E (or pT?)" };
  /// interface for xAOD::jet variable to be defined by user, this must correspond to jet Eta in currect version of jet calibration files
  ToolHandle<JetHelper::IVarTool> m_vartoolEta{this, "VarToolEta", "VarTool", "InputVariable instance eta (or rapididty?)" };

  // protected:
  unsigned int m_nPar{}; // number of parameters in config file
  const static unsigned int s_nEtaBins = 90;
  const static unsigned int s_nParMax = 9; 
  double m_JESFactors[s_nEtaBins][s_nParMax]{};
  double m_etaCorrFactors[s_nEtaBins][s_nParMax]{};
  double m_energyFreezeJES[s_nEtaBins]{};
  double m_JES_MinPt_Slopes[s_nEtaBins]={};
  double m_JES_MinPt_E[s_nEtaBins]={};
  double m_JES_MinPt_R[s_nEtaBins]={};


  TAxis * m_etaBinAxis{};

  std::vector<std::unique_ptr<TH1> > m_etajesFactors;

  
};

#endif

