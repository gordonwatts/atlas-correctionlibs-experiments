/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#ifndef JetMomentTools_JetNumConstitTool_H
#define JetMomentTools_JetNumConstitTool_H

/// Tool to decorate the number of constituents to jets
/// to not have to rely on the presence of constituentLinks
/// in output formats where the constituents are not stored

#include "JetInterface/IJetDecorator.h"
#include "AsgTools/AsgTool.h"
#include "AsgTools/PropertyWrapper.h"
#include "AsgDataHandles/WriteDecorHandleKey.h"

class JetNumConstitTool : public asg::AsgTool,
			  virtual public IJetDecorator {
  ASG_TOOL_CLASS(JetNumConstitTool, IJetDecorator)

public:

  // Constructor from tool name.
  JetNumConstitTool(const std::string& myname);

  virtual StatusCode initialize() override;

  // Inherited method to decorate a jet container.
  // Calls width and puts the result on the jets.
  virtual StatusCode decorate(const xAOD::JetContainer& jets) const override;

private:

  Gaudi::Property<std::string> m_jetContainerName{this, "JetContainer", "", "SG key for the input jet container"};

  SG::WriteDecorHandleKey<xAOD::JetContainer> m_numConstitKey{this, "numConstitKey", "numConstit", "SG key for number of constituents decoration (not including jet container name)"};

};

#endif
