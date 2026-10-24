///////////////////////// -*- C++ -*- /////////////////////////////

/*
  Copyright (C) 2002-2023 CERN for the benefit of the ATLAS collaboration
*/

// JetGroomMRatio.cxx 
// Implementation of jet modifier to compute mass ratio between groomed/un-groomed jets
/////////////////////////////////////////////////////////////////// 

#ifndef JETMOMENTTOOLS_JETGROOMMRATIO_H
#define JETMOMENTTOOLS_JETGROOMMRATIO_H

#include "AsgTools/AsgTool.h"
#include "AsgTools/PropertyWrapper.h"
#include "JetInterface/IJetDecorator.h"
#include "AsgDataHandles/WriteDecorHandleKey.h"
#include "xAODJet/JetContainer.h"

class JetGroomMRatio : public asg::AsgTool,
                        virtual public IJetDecorator {
  ASG_TOOL_CLASS0(JetGroomMRatio)
public:
  
  JetGroomMRatio(const std::string & t);

  virtual StatusCode initialize() override;
  virtual StatusCode decorate(const xAOD::JetContainer& jets) const override;

private:
  Gaudi::Property<std::string> m_jetContainerName{this, "JetContainer", "", "SG key for the input jet container"};

  SG::WriteDecorHandleKey<xAOD::JetContainer> m_groomMRatioKey{this, "groomMRatioName", "groomMRatio", "SG key for the groomMRatio attribute"};

};


#undef ASG_DERIVED_TOOL_CLASS
#endif

