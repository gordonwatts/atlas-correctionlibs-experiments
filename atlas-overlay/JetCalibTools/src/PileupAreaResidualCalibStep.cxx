/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "AsgDataHandles/ReadDecorHandle.h"

#include "JetCalibTools/IJetCalibrationTool.h"
#include "JetCalibTools/PileupAreaResidualCalibStep.h"
#include "JetCalibTools/JetCalibUtils.h"

#include "xAODJet/JetAccessors.h"

#include "PathResolver/PathResolver.h"

#include <memory>
#include <utility>

PileupAreaResidualCalibStep::PileupAreaResidualCalibStep(const std::string& name)
  : asg::AsgTool( name ) { }

StatusCode PileupAreaResidualCalibStep::initialize() {

  ATH_MSG_DEBUG("Reading from " << m_jetInScale << " and writing to " << m_jetOutScale);

  ATH_CHECK( m_muKey.initialize() );
  ATH_CHECK( m_pvKey.initialize() );
  ATH_CHECK( m_rhoKey.initialize() );

  if(m_doJetArea) ATH_MSG_DEBUG("Jet area pile up correction will be applied.");

  if(m_doSequentialResidual) ATH_MSG_INFO("The pileup residual calibrations will be applied sequentially.");
  else                       ATH_MSG_DEBUG("The pileup residual calibrations will be applied simultaneously (default).");

  if(m_doMuOnly)             ATH_MSG_INFO("Only the pileup mu-based calibration will be applied.");
  if(m_doNPVOnly)            ATH_MSG_INFO("Only the pileup NPV-based calibration will be applied.");

  // Protections
  CHECK_THEN_ERROR( m_doSequentialResidual && (m_doMuOnly || m_doNPVOnly) ,
		    "Sequential residual calibration can not be applied in doMuOnly or doNPVOnly cases.");

  CHECK_THEN_ERROR(m_doMuOnly && m_doNPVOnly,
		   "It was requested to apply only the mu-based AND the NPV-based calibrations.");

  CHECK_THEN_ERROR( (m_mu_ref==-99 && !m_doNPVOnly),
		    "OffsetCorrection.DefaultMuRef not specified.");

  CHECK_THEN_ERROR( (m_NPV_ref==-99 && !m_doMuOnly),
		    "OffsetCorrection.DefaultNPVRef not specified.");

  return StatusCode::SUCCESS;
}

StatusCode PileupAreaResidualCalibStep::calibrate(xAOD::JetContainer& jetCont) const {

  ATH_MSG_DEBUG("Calibrating jet collection with 1D pile-up correction");

  SG::ReadDecorHandle<xAOD::EventInfo,float> eventInfoDecor(m_muKey);
  CHECK_THEN_ERROR( ! eventInfoDecor.isPresent() , "EventInfo decoration not available! "<< m_muKey.key() );
  double mu = eventInfoDecor(0);

  SG::ReadHandle<xAOD::VertexContainer> PVCont(m_pvKey);
  CHECK_THEN_ERROR( ! PVCont.isValid() , "No Primary Vertices "<< m_pvKey.key() );
  double NPV = JetCalibUtils::countNPV(*PVCont);

  SG::ReadHandle<xAOD::EventShape> eventShape(m_rhoKey);
  CHECK_THEN_ERROR( ! eventShape.isValid() , "Could not retrieve xAOD::EventShape : "<< m_rhoKey.key());
  double rho=0;
  CHECK_THEN_ERROR( ! eventShape->getDensity(xAOD::EventShape::Density, rho ),
		  "Could not retrieve xAOD::EventShape::Density from xAOD::EventShape "<< m_rhoKey.key() );

  ATH_MSG_DEBUG("  Rho = " << 0.001*rho << " GeV");

  const xAOD::JetAttributeAccessor::AccessorWrapper<xAOD::JetFourMom_t> areaAcc("ActiveArea4vec");  
  const xAOD::JetAttributeAccessor::AccessorWrapper<xAOD::JetFourMom_t> outScaleMomAcc(m_jetOutScale);  
  const xAOD::JetAttributeAccessor::AccessorWrapper<xAOD::JetFourMom_t> startScaleMomAcc(m_jetInScale);  

  JetHelper::JetContext jc;

  for( xAOD::Jet * jet : jetCont){

    xAOD::JetFourMom_t jetStartP4 = startScaleMomAcc.getAttribute(*jet);

    const double E_det = jetStartP4.e();
    const double pT_det = jetStartP4.pt();
    const double mass_det = jetStartP4.mass();

    if ( E_det < mass_det ) {
      ATH_MSG_ERROR( "PileupAreaResidualCalibStep: jet has mass=" << mass_det << " MeV, which is greater than it's energy=" << E_det << " MeV! Aborting." );
      return StatusCode::FAILURE;
    }

    xAOD::JetFourMom_t jetareaP4 = areaAcc.getAttribute(*jet);
    ATH_MSG_VERBOSE("    Area = " << jetareaP4);

    double offsetET  = 0;  // pT residual subtraction
    double pT_offset = pT_det; // pT difference before/after pileup corrections
    double pileup_SF = 1; // final calibration factor applied to the four vector

    xAOD::JetFourMom_t calibP4;
    if(!m_doSequentialResidual){ // Default, both corrections are applied simultaneously
      offsetET = getResidualOffset(*jet, jc, mu, NPV, m_doMuOnly, m_doNPVOnly);

      // Calculate the pT after jet areas and residual offset
      pT_offset = m_doJetArea ? pT_det - rho*jetareaP4.pt() - offsetET : pT_det - offsetET;

      // Set the jet pT to 10 MeV if the pT is negative after the jet area and residual offset corrections
      pileup_SF = pT_offset >= 0 ? pT_offset / pT_det : 10./pT_det;

      calibP4 = jetStartP4*pileup_SF;

    }else{
      // Calculate mu-based correction factor
      offsetET = getResidualOffset(*jet, jc, mu, NPV, true, false);
      pT_offset   = m_doJetArea ? pT_det - rho*jetareaP4.pt() - offsetET : pT_det - offsetET;
      double muSF = pT_offset >= 0 ? pT_offset / pT_det : 10./pT_det;
      calibP4 = jetStartP4*muSF;

      // Calculate and apply NPV/Njet-based calibration
      offsetET = getResidualOffset(*jet, jc, mu, NPV, false, true);
      double pT_afterMuCalib = calibP4.pt();
      pT_offset = pT_afterMuCalib - offsetET;
      double SF = pT_offset >= 0 ? pT_offset / pT_afterMuCalib : 10./pT_afterMuCalib;
      calibP4   = calibP4*SF;
    }

    //Transfer calibrated jet properties to the jet
    outScaleMomAcc.setAttribute(*jet, calibP4 );
    jet->setJetP4( calibP4 );        
  }

  return StatusCode::SUCCESS;
}

double PileupAreaResidualCalibStep::getResidualOffset(const xAOD::Jet& jet, const JetHelper::JetContext& jc, double mu, double NPV, bool MuOnly, bool NPVOnly) const{

  double alpha = 0.0, beta = 0.0;

  if(!NPVOnly){
    alpha = m_histTool_mu->getValue(jet, jc);
  }
  if(!MuOnly){
    beta = m_histTool_NPV->getValue(jet, jc);
  }

  static const double toMeV= 1000.;
  //mu rescaling
  const double muCorr = m_isData ? mu : mu*m_muSF;

  return (alpha*(muCorr-m_mu_ref) + beta*(NPV-m_NPV_ref))*toMeV;   
}
