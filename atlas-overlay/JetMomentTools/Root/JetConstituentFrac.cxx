/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "JetMomentTools/JetConstituentFrac.h"

#include "xAODJet/JetAccessorMap.h"
#include "AsgDataHandles/WriteDecorHandle.h"

#include "xAODPFlow/FlowElement.h"

//**********************************************************************

JetConstituentFrac::JetConstituentFrac(const std::string& name)
: AsgTool(name) { }

//**********************************************************************

StatusCode JetConstituentFrac::initialize() {
  ATH_MSG_DEBUG("Initializing JetConstituentFrac " << name());
  
  if(m_jetContainerName.empty()){
    ATH_MSG_ERROR("JetConstituentFrac needs to have its input jet container configured!");
    return StatusCode::FAILURE;
  }
  
  m_neutralEFracKey = m_jetContainerName + "." + m_neutralEFracKey.key();
  m_chargePTFracKey = m_jetContainerName + "." + m_chargePTFracKey.key();
  m_chargeMFracKey = m_jetContainerName + "." + m_chargeMFracKey.key();

  ATH_CHECK(m_neutralEFracKey.initialize());
  ATH_CHECK(m_chargePTFracKey.initialize());
  ATH_CHECK(m_chargeMFracKey.initialize());

  return StatusCode::SUCCESS;
}

//**********************************************************************

StatusCode JetConstituentFrac::decorate(const xAOD::JetContainer& jets) const {
  ATH_MSG_VERBOSE("Begin decorating jets.");
  for(const xAOD::Jet* jet : jets) {
    
    fillConstituentFrac(*jet);

  }
  return StatusCode::SUCCESS;
}

void JetConstituentFrac::fillConstituentFrac(const xAOD::Jet& jet) const {

  float neutE=0;
  float chargPt=0;
  TLorentzVector chargM = {0., 0., 0., 0.};

  size_t numConstit = jet.numConstituents();
  for ( size_t i=0; i<numConstit; i++ ) {
    if(jet.rawConstituent(i)->type()!=xAOD::Type::FlowElement) {
      ATH_MSG_WARNING("Tried to call fillConstituentFrac with a jet constituent that is not a FlowElement!");
      continue;
    }
    const xAOD::FlowElement* constit = static_cast<const xAOD::FlowElement*>(jet.rawConstituent(i));

    if(constit->signalType() == xAOD::FlowElement::Neutral) neutE+= constit->e();    
    if (constit->signalType() == xAOD::FlowElement::Charged){
      chargPt += constit->pt();
      chargM += constit->p4();
    }
  }

  SG::WriteDecorHandle<xAOD::JetContainer, float> neutralEFracHandle(m_neutralEFracKey);
  SG::WriteDecorHandle<xAOD::JetContainer, float> chargePTFracHandle(m_chargePTFracKey);
  SG::WriteDecorHandle<xAOD::JetContainer, float> chargeMFracHandle(m_chargeMFracKey);

  // Get jet p4 at constituent scale to compute ratios
  xAOD::JetFourMom_t uncalP4;
  jet.getAttribute<xAOD::JetFourMom_t>("JetConstitScaleMomentum",uncalP4);
  
  neutralEFracHandle(jet)  = neutE/uncalP4.e();
  chargePTFracHandle(jet)  = chargPt/uncalP4.pt();
  chargeMFracHandle(jet)   = uncalP4.M() != 0. ? chargM.M()/uncalP4.M() : 0.;

}
