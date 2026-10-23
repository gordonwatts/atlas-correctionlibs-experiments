class Nominal;
///////////////////////// -*- C++ -*- /////////////////////////////

/*
  Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration
*/

// JetCalibrationTool.h 
// Header file for class JetCalibrationTool
/////////////////////////////////////////////////////////////////// 
#ifndef JETCALIBTOOLS_APPLYJETCALIBRATION_H
#define JETCALIBTOOLS_APPLYJETCALIBRATION_H 1

#include <string.h>

#include <TString.h>
#include <TEnv.h>

#include "AsgTools/AsgMetadataTool.h"
#include "AsgTools/AsgToolMacros.h"

#include "xAODEventInfo/EventInfo.h"
#include "xAODTracking/VertexContainer.h"
#include "xAODEventShape/EventShape.h"
#include "AsgDataHandles/ReadDecorHandleKey.h"

#include "JetCalibTools/IJetCalibrationTool.h"
#include "JetCalibTools/JetEventInfo.h"
#include "JetCalibTools/JetCalibrationStep.h"


class JetCalibrationTool
  : public asg::AsgMetadataTool,
    virtual public IJetCalibrationTool {

  ASG_TOOL_CLASS2(JetCalibrationTool, IJetCalibrationTool, IJetModifier)

public:
  /// Constructor with parameters: 
  JetCalibrationTool(const std::string& name = "JetCalibrationTool");

  /// Destructor: 
  ~JetCalibrationTool(); 

  enum jetScale { EM, LC, PFLOW };

  StatusCode initialize() override;

  StatusCode applyCalibration(xAOD::JetContainer&) const override;

  // Get the nominal resolution
  StatusCode getNominalResolutionData(const xAOD::Jet& jet, double& resolution) const override;
  StatusCode getNominalResolutionMC(  const xAOD::Jet& jet, double& resolution) const override;
  StatusCode getNominalResolutionHist( const xAOD::Jet& jet, double& resolution, const TH2D* histo) const;
  
private:
  std::shared_ptr<::Nominal> m_correctionBackend;
  StatusCode calibrate(xAOD::Jet& jet, JetEventInfo& jetEventInfo) const;
  //Set event by event info like rho, mu, NPV
  StatusCode initializeEvent(JetEventInfo& jetEventInfo) const;
  //Add an item to the vector of calibration steps
  StatusCode getCalibClass(const TString& calibration);

//Private members
private:
  SG::ReadHandleKey<xAOD::EventInfo> m_evtInfoKey{this, "EventInfoKey", "EventInfo"};
  SG::ReadHandleKey<xAOD::EventShape> m_rhoKey{this, "RhoKey", "auto"};
  SG::ReadHandleKey<xAOD::VertexContainer> m_pvKey{this, "PrimaryVerticesContainerName", "PrimaryVertices"};
  SG::ReadDecorHandleKey<xAOD::EventInfo> m_muKey {this, "averageInteractionsPerCrossingKey",
          "EventInfo.averageInteractionsPerCrossing","Decoration for Average Interaction Per Crossing"};
  SG::ReadDecorHandleKey<xAOD::EventInfo> m_actualMuKey {this, "actualInteractionsPerCrossing",
          "EventInfo.actualInteractionsPerCrossing","Decoration for Actual Number of Interactions Per Crossing"};

  //Variables for configuration
  std::string m_jetAlgo;
  std::string m_config;
  std::string m_calibSeq;
  std::string m_calibAreaTag;
  std::string m_originScale;
  bool m_devMode{};
  bool m_isData{true};
  std::string m_forceCampaign{};
  bool m_timeDependentCalib{};
  bool m_originCorrectedClusters{};
  bool m_useNjetInResidual{};
  float m_nJetThreshold{};
  std::string m_nJetContainerName;
  std::string m_dir;
  std::string m_eInfoName;
  std::vector<TString> m_timeDependentInsituConfigs;
  std::vector<double>  m_runBins;
  bool m_doSetDetectorEta{};
  bool m_insituCombMassCalib{};
  std::vector<TString> m_insituCombMassConfig;
  std::string m_forceCalibFile_PtResidual{};
  std::string m_forceCalibFile_FastSim{};
  std::string m_forceCalibFile_MC2MC{};

  //TEnv to hold the global text config
  TEnv * m_globalConfig{};
  std::vector<TEnv*> m_globalTimeDependentConfigs;
  std::vector<TEnv*> m_globalInsituCombMassConfig;

  //Bools/enums to avoid string comparisons at run time
  jetScale m_jetScale = jetScale::EM;
  bool m_doBcid{true};
  bool m_doJetArea{true};
  bool m_doResidual{true};
  bool m_doOrigin{true};
  bool m_doGSC{true};
  bool m_doDNNCal{};

  std::string m_gscDepth{"auto"}; // if not set then use the one defined in the config

  std::vector<std::unique_ptr<JetCalibrationStep> > m_calibSteps;
  int m_smearIndex{-1};

  // Try to use jet-attribute-specified origin vertex for calibration
  bool m_useOriginVertex{};
}; 

#endif //> !JETCALIBTOOLS_APPLYJETCALIBRATION_H
