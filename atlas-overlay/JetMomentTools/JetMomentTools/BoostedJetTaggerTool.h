/*
 * Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration
 */

#ifndef DERIVATIONFRAMEWORK_BOOSTEDJETTAGGERTOOL_H
#define DERIVATIONFRAMEWORK_BOOSTEDJETTAGGERTOOL_H

#include "BoostedJetTaggers/JSSTaggerUtils.h"

#include "JetInterface/IJetDecorator.h"
#include "AsgTools/ToolHandle.h"
#include "AsgTools/PropertyWrapper.h"

class BoostedJetTaggerTool : public asg::AsgTool,
			     virtual public IJetDecorator{
  ASG_TOOL_CLASS(BoostedJetTaggerTool,IJetDecorator)
    
  public:
  
  BoostedJetTaggerTool(const std::string & name);
  
  virtual StatusCode initialize() override;
  
  virtual StatusCode decorate(const xAOD::JetContainer& jets) const override;
  
private:

  // Properties
  ToolHandle<IJSSTaggerUtils> m_MLTagger{this, "MLTagger", "", "ToolHandle for IJSSTaggerUtils"};
  Gaudi::Property<std::string> m_jetContainerName{this, "JetContainer", "", "SG key of input jet container"};
  Gaudi::Property<std::string> m_decorationName{this, "DecorationName", "", "Tagger decoration name key"};
  Gaudi::Property<std::string> m_calibArea{this, "CalibArea", "", "CalibArea key"};
  Gaudi::Property<std::string> m_configFile{this, "ConfigFile", "", "ConfigFile key"};

  Gaudi::Property<bool> m_suppressInputDependence{this, "SuppressInputDependence", false};
  
  // Internals
  
}; //> end class BoostedJetTaggerTool

#endif //> !JETMOMENTTOOLS_BOOSTEDJETTAGGERTOOL_H
