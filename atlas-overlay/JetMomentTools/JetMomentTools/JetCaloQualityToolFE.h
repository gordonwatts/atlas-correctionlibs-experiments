// this file is -*- C++ -*-

/*
  Copyright (C) 2002-2023 CERN for the benefit of the ATLAS collaboration
*/

#ifndef JETMOMENTTOOLS_JETCALOQUALITYTOOLFE_H
#define JETMOMENTTOOLS_JETCALOQUALITYTOOLFE_H

#include "AsgTools/AsgTool.h"
#include "AsgTools/PropertyWrapper.h"
#include "JetInterface/IJetDecorator.h"
#include "AsgDataHandles/WriteDecorHandleKeyArray.h"

#include "xAODCaloEvent/CaloCluster.h"

#include <vector>
#include <string>


class JetCaloQualityToolFE: public asg::AsgTool,
                          virtual public IJetDecorator {
  ASG_TOOL_CLASS1(JetCaloQualityToolFE,IJetDecorator)
  
public:
  JetCaloQualityToolFE(const std::string & name);

  virtual StatusCode decorate(const xAOD::JetContainer& jets) const override;
  
  virtual StatusCode initialize() override;

 protected:

  Gaudi::Property<std::vector<std::string> > m_calculationNames{this, "Calculations", {},
      "Name of calo quantities to compute and add as decorations"};
  Gaudi::Property<std::vector<double> > m_timingTimeCuts{this, "TimingCuts", {},
      "Time cuts for out-of-time calo quantities"};
  Gaudi::Property<std::vector<int> > m_thresholdCuts{this, "ThresholdCuts", {},
      "Thresholds cuts (NxConstituents)"};
  Gaudi::Property<std::string> m_jetContainerName{this, "JetContainer", "",
      "SG key of input jet container"};

  SG::WriteDecorHandleKeyArray<xAOD::JetContainer> m_writeDecorKeys{this, "OutputDecorKeys", {},
      "SG keys for output decorations (not to be configured manually!)"};

  SG::WriteDecorHandleKeyArray<xAOD::JetContainer> m_writeDecorKeys_OOT{this, "OutputOOTDecorKeys", {},
      "SG keys for output OOT decorations (not to be configured manually!)"};

  SG::WriteDecorHandleKeyArray<xAOD::JetContainer> m_writeDecorKeys_Nfrac{this, "OutputNfracConstitDecorKeys", {},
      "SG keys for output NfracConstituents decorations (not to be configured manually!)"};

  void fillQualityVariables(const xAOD::Jet &jet) const ;

  std::vector<const xAOD::CaloCluster*> extractConstituents(const xAOD::Jet& jet) const ;

  bool m_doLArQ = false;
  bool m_doHECQ = false;
  bool m_doNegE = false;
  bool m_doAvgLAr = false;
  bool m_doCentroid = false;
  bool m_doBchCorrCell = false;
  bool m_doTime = false;

};
#endif 

