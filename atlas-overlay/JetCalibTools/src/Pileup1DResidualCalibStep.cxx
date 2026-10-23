/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "AsgDataHandles/ReadDecorHandle.h"

#include "JetCalibTools/IJetCalibrationTool.h"
#include "JetCalibTools/Pileup1DResidualCalibStep.h"
#include "JetCalibTools/JetCalibUtils.h"
#include "JetCalibTools/CalibrationMethods/NPVBeamspotCorrection.h"

#include "xAODJet/JetAccessors.h"


#include "PathResolver/PathResolver.h"

#include <memory>
#include <utility>


Pileup1DResidualCalibStep::Pileup1DResidualCalibStep(const std::string& name)
  : asg::AsgTool( name ) { }




StatusCode Pileup1DResidualCalibStep::initialize() {

  if(m_doJetArea && m_doOnlyResidual){
    ATH_MSG_FATAL("If you're trying to apply only the Residual pile up correction, it needs to be specify in the Calibration Sequence. ApplyOnlyResidual should be true in the configuration file and the PileupStartScale should be specified.");
    return StatusCode::FAILURE;
  }

  ATH_MSG_DEBUG("Reading from " << m_jetInScale << " and writing to " << m_jetOutScale);
  
  ATH_CHECK( m_muKey.initialize() );
  ATH_CHECK( m_pvKey.initialize() );
  ATH_CHECK( m_rhoKey.initialize() );
  ATH_CHECK(m_nJetContainerKey.initialize(SG::AllowEmpty));

  
  
  if(m_doSequentialResidual) ATH_MSG_DEBUG("The pileup residual calibrations will be applied sequentially.");
  else                       ATH_MSG_DEBUG("The pileup residual calibrations will be applied simultaneously (default).");
  if(m_doMuOnly)             ATH_MSG_INFO("Only the pileup mu-based calibration will be applied.");
  if(m_doNPVOnly)            ATH_MSG_INFO("Only the pileup NPV-based calibration will be applied.");
  if(m_doNJetOnly)           ATH_MSG_INFO("Only the pileup NJet-based calibration will be applied.");
  if (m_useNjet)             ATH_MSG_DEBUG("NJet will be used instead of NPV in the pileup corrections.");

  // Protections
  CHECK_THEN_ERROR( m_doSequentialResidual && (m_doMuOnly || m_doNPVOnly || m_doNJetOnly) ,
		    "Sequential residual calibration can not be applied in doMuOnly or doNPVOnly or doNJetOnly cases.");
  
  CHECK_THEN_ERROR(m_useNjet && (m_doMuOnly || m_doNPVOnly),
		   "Conflicting configuation, UseNjet true but doMuOnly or doNPVOnly also true.");
  
  CHECK_THEN_ERROR(m_doMuOnly && m_doNPVOnly,
		   "It was requested to apply only the mu-based AND the NPV-based calibrations.");
    
  CHECK_THEN_ERROR(m_doMuOnly && m_doNJetOnly, 
		   "It was requested to apply only the mu-based AND the NJet-based calibrations.");

  CHECK_THEN_ERROR(!m_useNjet && m_doNJetOnly, 
		   "It was requested to apply only the NJet-based calibration but not to use Njet instead of NPV.");

  CHECK_THEN_ERROR(m_doNJetOnly && m_doNPVOnly, 
		   "It was requested to apply NJet-based and NPV calibrations.");

  if(m_doJetArea) ATH_MSG_DEBUG("Jet area pile up correction will be applied.");


  CHECK_THEN_ERROR( (m_mu_ref==-99 && !m_doNPVOnly && !m_doNJetOnly),
		    "OffsetCorrection.DefaultMuRef not specified.");
    
  CHECK_THEN_ERROR( (m_NPV_ref==-99 && !m_doMuOnly && !m_useNjet),
		    "OffsetCorrection.DefaultNPVRef not specified.");
    
  CHECK_THEN_ERROR( (m_nJet_ref==-99 && m_useNjet),
		    "OffsetCorrection.DefaultNjetRef not specified.");

  
  m_resOffsetBins = new TAxis(m_offsetEtaBins.size()-1,&m_offsetEtaBins[0]);

  if(!m_doNPVOnly && !m_doNJetOnly){  
    CHECK_THEN_ERROR( (int)m_resOffsetMu.size()!=(m_resOffsetBins->GetNbins()+1),
		      "Incorrect specification of MuTerm binning"  );
  }
  
  if(m_useNjet) {
    CHECK_THEN_ERROR( (int)m_resOffsetNjet.size()!= (m_resOffsetBins->GetNbins()+1), 
		      "Incorrect specification of nJetTerm binning."  );
  } else if(!m_doMuOnly){
    CHECK_THEN_ERROR( (int)m_resOffsetNPV.size()!= (m_resOffsetBins->GetNbins()+1), 
		      "Incorrect specification of NPVTerm binning."  );

    if( m_applyNPVBeamspotCorrection) {
      m_npvBeamspotCorr = new NPVBeamspotCorrection();
      ATH_MSG_INFO("\n  NPV beamspot correction will be applied.");
    } 
  }
  
  return StatusCode::SUCCESS;
}



StatusCode Pileup1DResidualCalibStep::calibrate(xAOD::JetContainer& jetCont) const {

  ATH_MSG_DEBUG("Calibrating jet collection with 1D pile-up correction");

  SG::ReadDecorHandle<xAOD::EventInfo,float> eventInfoDecor(m_muKey);
  CHECK_THEN_ERROR( ! eventInfoDecor.isPresent() , "EventInfo decoration not available! "<< m_muKey.key() );
  double mu = eventInfoDecor(0) ;
  
  
  SG::ReadHandle<xAOD::VertexContainer> PVCont(m_pvKey);
  CHECK_THEN_ERROR( ! PVCont.isValid() , "No Primary Vertices "<< m_pvKey.key() );
  double NPV = JetCalibUtils::countNPV(*PVCont);

  SG::ReadHandle<xAOD::EventShape> eventShape(m_rhoKey);
  CHECK_THEN_ERROR( ! eventShape.isValid() , "Could not retrieve xAOD::EventShape : "<< m_rhoKey.key());
  double rho=0;
  CHECK_THEN_ERROR( ! eventShape->getDensity(xAOD::EventShape::Density, rho ),
		  "Could not retrieve xAOD::EventShape::Density from xAOD::EventShape "<< m_rhoKey.key() );

  int nJets=0;
  if(m_useNjet){
    SG::ReadHandle<xAOD::JetContainer> nJetCont(m_nJetContainerKey);
    CHECK_THEN_ERROR( !nJetCont.isValid() , "Jet container needed for nJet not found: "<< m_nJetContainerKey.key());
    for(const xAOD::Jet*jet: *nJetCont){
      if(jet->pt()/1000. > m_njetThreshold)
	nJets += 1;
    }      
  }


  ATH_MSG_DEBUG("  Rho = " << 0.001*rho << " GeV");
  static const double toGeV = 0.001;
  const xAOD::JetAttributeAccessor::AccessorWrapper<xAOD::JetFourMom_t> areaAcc("ActiveArea4vec");  
  const xAOD::JetAttributeAccessor::AccessorWrapper<xAOD::JetFourMom_t> outScaleMomAcc(m_jetOutScale);  
  const xAOD::JetAttributeAccessor::AccessorWrapper<xAOD::JetFourMom_t> startScaleMomAcc(m_jetInScale);  
 
  for( xAOD::Jet * jet : jetCont){

    xAOD::JetFourMom_t jetStartP4 = startScaleMomAcc.getAttribute(*jet);
    
    const double E_det = jetStartP4.e();
    const double pT_det = jetStartP4.pt();
    const double eta_det = jetStartP4.eta();
    const double mass_det = jetStartP4.mass();

    if ( E_det < mass_det ) {
      ATH_MSG_ERROR( "JetPileupCorrection::calibrateImpl : Current jet has mass=" << mass_det*toGeV << " GeV, which is greater than it's energy=" << E_det*toGeV << " GeV?? Aborting." );
      return StatusCode::FAILURE;
    }

    xAOD::JetFourMom_t jetareaP4 = areaAcc.getAttribute(*jet);
    ATH_MSG_VERBOSE("    Area = " << jetareaP4);

    double offsetET  = 0;  // pT residual subtraction
    double pT_offset = pT_det; // pT difference before/after pileup corrections
    double pileup_SF = 1; // final calibration factor applied to the four vector

    xAOD::JetFourMom_t calibP4;
    if(!m_doSequentialResidual){ // Default, both corrections are applied simultaneously

      offsetET = getResidualOffset(fabs(eta_det), mu, NPV, nJets, m_doMuOnly, m_doNPVOnly||m_doNJetOnly);

      // Calculate the pT after jet areas and residual offset
      pT_offset = m_doJetArea ? pT_det - rho*jetareaP4.pt() - offsetET : pT_det - offsetET;

      // Set the jet pT to 10 MeV if the pT is negative after the jet area and residual offset corrections
      pileup_SF = pT_offset >= 0 ? pT_offset / pT_det : 10./pT_det;

      calibP4 = jetStartP4*pileup_SF;
      
    }else{
      // Calculate mu-based correction factor
      offsetET    = getResidualOffset(fabs(eta_det), mu, NPV, nJets, true, false);
      pT_offset   = m_doJetArea ? pT_det - rho*jetareaP4.pt() - offsetET : pT_det - offsetET;
      double muSF = pT_offset >= 0 ? pT_offset / pT_det : 10./pT_det;

      calibP4 = jetStartP4*muSF;

      // Calculate and apply NPV/Njet-based calibration
      offsetET = getResidualOffset(fabs(eta_det), mu, NPV, nJets, false, true);
      double pT_afterMuCalib = calibP4.pt();
      pT_offset = pT_afterMuCalib - offsetET;
      double SF = pT_offset >= 0 ? pT_offset / pT_afterMuCalib : 10./pT_afterMuCalib;
      calibP4   = calibP4*SF;
      
    }

    //Transfer calibrated jet properties to the Jet object
    outScaleMomAcc.setAttribute(*jet, calibP4 );
    jet->setJetP4( calibP4 );        
    
  }
  
  
  return StatusCode::SUCCESS;
}


double Pileup1DResidualCalibStep::getResidualOffset ( double abseta, double mu, double NPV,
                                                     int nJet, bool MuOnly, bool NOnly ) const {
  return getResidualOffsetET(abseta, mu, NPV, nJet, MuOnly, NOnly,
                             m_resOffsetMu, m_resOffsetNPV, m_resOffsetNjet, m_resOffsetBins);
}

double Pileup1DResidualCalibStep::getResidualOffsetET( double abseta, double mu, double NPV,
                                                      int nJet, bool MuOnly, bool NOnly,
                                                      const std::vector<double>& OffsetMu,
                                                      const std::vector<double>& OffsetNPV,
                                                      const std::vector<double>& OffsetNjet,
                                                      const TAxis *OffsetBins ) const {

  static const double toMeV= 1000.;
  //mu rescaling
  const double muCorr = m_isData ? mu : mu*m_muSF;
  if(m_useNjet) {
    // further correction to nJet if desired
    int nJetCorr = nJet;

    double alpha, beta, etaEdge;
    if(!MuOnly){ beta    = OffsetNjet[0];
    } else { beta  = 0; }
    if(!NOnly){ alpha = OffsetMu[0];
    } else { alpha = 0; }
    etaEdge=0;
    int bin=1;
    for (;bin<=OffsetBins->GetNbins();++bin) {
      etaEdge = OffsetBins->GetBinLowEdge(bin);
      const double width=OffsetBins->GetBinWidth(bin);
      if (abseta<etaEdge+width) break;
      if(!NOnly)  alpha += width*OffsetMu[bin];
      if(!MuOnly) beta  += width*OffsetNjet[bin];
    }
    if(!NOnly)  alpha += OffsetMu[bin]*(abseta-etaEdge);
    if(!MuOnly) beta  += OffsetNjet[bin]*(abseta-etaEdge);
    return (alpha*(muCorr-m_mu_ref) + beta*(nJetCorr-m_nJet_ref))*toMeV;
  } else {
    //NPV beamspot correction
    const double NPVCorr = getNPVBeamspotCorrection(NPV);

    double alpha, beta, etaEdge;
    if(!MuOnly){ beta   = OffsetNPV[0];
    } else { beta  = 0; }
    if(!NOnly){ alpha = OffsetMu[0];
    } else { alpha = 0; }
    etaEdge = 0;
    int bin=1;
    for (;bin<=OffsetBins->GetNbins();++bin) {
      etaEdge = OffsetBins->GetBinLowEdge(bin);
      const double width=OffsetBins->GetBinWidth(bin);
      if (abseta<etaEdge+width) break;
      if(!NOnly)  alpha += width*OffsetMu[bin];
      if(!MuOnly) beta  += width*OffsetNPV[bin];
    }
    if(!NOnly)  alpha += OffsetMu[bin]*(abseta-etaEdge);
    if(!MuOnly) beta  += OffsetNPV[bin]*(abseta-etaEdge);
    return (alpha*(muCorr-m_mu_ref) + beta*(NPVCorr-m_NPV_ref))*toMeV;
  }
  
}

double Pileup1DResidualCalibStep::getNPVBeamspotCorrection(double NPV) const {
  if(!m_isData && m_applyNPVBeamspotCorrection) return std::as_const(m_npvBeamspotCorr)->GetNVertexBsCorrection(NPV);
  return NPV;
}
