///////////////////////// -*- C++ -*- /////////////////////////////

/*
  Copyright (C) 2002-2023 CERN for the benefit of the ATLAS collaboration
*/

// JetConstituentFrac
// Implementation of jet modifier to compute energy fractions of charge/neutral constituents
/////////////////////////////////////////////////////////////////// 

#ifndef JETMOMENTTOOLS_JETCONSTITUENTFRAC_H
#define JETMOMENTTOOLS_JETCONSTITUENTFRAC_H

#include "AsgTools/AsgTool.h"
#include "AsgTools/PropertyWrapper.h"
#include "JetInterface/IJetDecorator.h"
#include "AsgDataHandles/WriteDecorHandleKey.h"
#include "xAODJet/JetContainer.h"

class JetConstituentFrac : public asg::AsgTool,
                        virtual public IJetDecorator {
  ASG_TOOL_CLASS0(JetConstituentFrac)
public:
  
  JetConstituentFrac(const std::string & t);

  virtual StatusCode initialize() override;
  virtual StatusCode decorate(const xAOD::JetContainer& jets) const override;

protected:
  void fillConstituentFrac(const xAOD::Jet &jet) const ;

private:
  Gaudi::Property<std::string> m_jetContainerName{this, "JetContainer", "", "SG key for the input jet container"};

  SG::WriteDecorHandleKey<xAOD::JetContainer> m_neutralEFracKey{this, "NeutralEFracName", "NeutralEFrac", "SG key for the NeutralEFrac attribute"};
  SG::WriteDecorHandleKey<xAOD::JetContainer> m_chargePTFracKey{this, "ChargePTFracName", "ChargePTFrac", "SG key for the ChargePTFrac attribute"};
  SG::WriteDecorHandleKey<xAOD::JetContainer> m_chargeMFracKey{this, "ChargeMFracName", "ChargeMFrac", "SG key for the ChargeMFrac attribute"};

};


#undef ASG_DERIVED_TOOL_CLASS
#endif

