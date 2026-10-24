/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

// JetIsolationTool.h 
//
// Author: P-A. Delsart
//

#ifndef JetIsolationTool_H
#define JetIsolationTool_H

/////////////////////////////////////////////////////////////////// 
/// \class JetIsolationTool
///
/// Calculate isolation variables for jets and set them as jet attributes.
///
/// Properties:
///   IsolationCalculations: List of calculations to perform (see below)
///   InputConstitContainer: name  of the jet constituents container
///   RhoKey: EventDensity key. Required in case the PtPUsub calculation is performed.
///
/// The isolation variables are calculated from a list of input constituents.
/// which are close to the jet but not part of its constituents.
/// There are multiple options to find this list and then to calculate the  
/// variables.
/// They are passed to the tools by giving a list of string identifiers through
/// the IsolationCalculations property as in
///
///  IsolationCalculations = ["IsoKR:11:Perp","IsoKR:11:Par", "IsoFixedCone:10:SumPt",]
///
/// where the strings have the form "ISOCRITERIA:NN:VAR"
///  - ISOCRITERIA : encode the method used to find isolation particles (IsoKR, IsoDelta, IsoFixedCone, IsoFixedArea, Iso6To8, signification detailed below)
///  - NN : encode the value of the main parameter of ISOCRITERIA : the actual parameter used is NN/10. (same convetion as jet radius in JetContainer names)
///  - VAR : encode the variable to be calculated (Per, Par, SumPt, P, significations detailed below)
///
/// The resulting attribute name is simply the identifier string with ':' removed. 
/// Example "IsoKR:11:Perp" -> "IsoKR11Perp"
/// 
/// The input particles container from which constituents come from must be specified 
/// in the ConstituentContainer property. This must correspond to an IParticleContainer in the event store.
/// (longer term : make it possible to pass PseudoJetContainer)
///
///
///
/// Because of the multiplicity of possible isolation and calculation choices, the internal code is modularized.
/// The code is potentially slow (~O(Nconstit x Njet x Nisovariable)) and could be improved if needed by using fast look-up map and/or preselecting constituents.
///
/// WARNING: The constituents must be the *same* in-memory objects as the jet constituents. 
/// 
/// Isolation Criteria ("param" below is the main parameter) :
///  - IsoKR    : iso area == cone of size __jetRadius*param__
///  - IsoDelta : iso area == cone of size __jetRadius+param__
///  - IsoFixedCone : iso are == cone of size __param__
///  - IsoFixedArea : iso are == cone of size __sqrt(jetRadius*jetRadius+param/M_PI)__
///  - Iso6To8 : iso area == annulus between R=0.6 and R=0.8
///
/// Isolation Variables (iso4vec = sum of 4-vec in isolation area, not part of jet)
///  - Pt : iso4vec.Pt/ jetPt 
///  - PtPUsub : (iso4vec.Pt-rho*isoArea)/ (jetPt - rho*jetArea)
///  - Perp : iso4Vec.Perp( jetDirection )/ jetPt 
///  - Par  : iso4Vec.Dot( jetDirection )/ jetPt 
///  - SumPt : sum Pt of 4-vec contibuting to iso4Vec / jetPt 
///  - P : iso4Vec.Vect().Mag()/ jetPt 
///////////////////////////////////////////////////////////////////////

#include "AsgTools/AsgTool.h"
#include "JetUtils/TiledEtaPhiMap.h"
#include "JetInterface/IJetDecorator.h"
#include "fastjet/PseudoJet.hh"
#include "JetEDM/IConstituentUserInfo.h"
#include "StoreGate/ReadDecorHandleKey.h"
#include "StoreGate/ReadDecorHandle.h"
#include "StoreGate/WriteDecorHandleKey.h"
#include "StoreGate/WriteDecorHandleKeyArray.h"
#include "StoreGate/WriteDecorHandle.h"
#include "xAODJet/JetContainer.h"
#include "xAODBase/IParticleContainer.h"
#include "xAODEventShape/EventShape.h"

namespace jet {

namespace JetIsolation {
  class IsolationCalculator;
}


}  // end namespace jet

//**********************************************************************

class JetIsolationTool : public asg::AsgTool,
                         virtual public IJetDecorator { 
  ASG_TOOL_CLASS0(JetIsolationTool)

public: 

  friend class jet::JetIsolation::IsolationCalculator;

  /// Ctor.
  JetIsolationTool(const std::string &myname);

  /// Dtor.
  virtual ~JetIsolationTool(); 

  // Athena algtool Hooks
  virtual StatusCode  initialize() override;
  virtual StatusCode  finalize() override;

  // Jet Modifier methods.
  virtual StatusCode decorate(const xAOD::JetContainer& jets) const override;

private: 
  Gaudi::Property<std::vector<std::string>> m_isolationCodes{this, "IsolationCalculations", {}, "Isolation calculations to be performed"};
  Gaudi::Property<std::string> m_jetContainerName{this, "JetContainer", "", "SG key for the input jet container"};

  SG::ReadHandleKey<xAOD::IParticleContainer> m_inputConstitKey{this, "InputConstitContainer", "", "Constituent container to read (should be configured automatically from a JetDefinition)"};


  SG::WriteDecorHandleKeyArray<xAOD::JetContainer> m_decorKeys{this, "DecorationKeys", {}, "SG key for output momentum decoration (not to be configured manually!)"};

  SG::ReadHandleKey<xAOD::EventShape> m_rhoKey{this, "RhoKey", "", "EventDensity for this jet collection. Required only if PtPUsub calculation is done"};
  
  /// the list of isolation calculation objects (they are actually used
  /// only as template objects from which the actual calculators are build
  // and adapted to the jet object, see implementation)
  std::vector<jet::JetIsolation::IsolationCalculator*> m_isoCalculators;


  
}; 

#endif
