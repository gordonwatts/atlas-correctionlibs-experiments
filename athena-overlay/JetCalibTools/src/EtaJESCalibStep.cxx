///////////////////////// -*- C++ -*- /////////////////////////////

/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// EtaJESCalibStep.cxx 
// Implementation file for class EtaJESCalibStep
/////////////////////////////////////////////////////////////////// 

#include "JetCalibTools/EtaJESCalibStep.h"
#include "PathResolver/PathResolver.h"

#include "TFile.h"

using JetCalibUtils::VectorizeD;

EtaJESCalibStep::EtaJESCalibStep(const std::string& name)
  : asg::AsgTool( name ){ }


StatusCode EtaJESCalibStep::initialize(){
  ATH_MSG_DEBUG ("Initializing " << name() << " Use spline="<<m_useSpline);

  ATH_MSG_DEBUG("Reading from " << m_jetInScale << " and writing to " << m_jetOutScale);

  if(! m_useSpline){
    if(!readMCJESFromText()) {
      ATH_MSG_ERROR("Problem when reading constant file : "<< m_constantFileName);
      return StatusCode::FAILURE;
    }
  } else { // use spline 
    if(!readMCJESFromHists()) {
      ATH_MSG_ERROR("Problem when reading constant file : "<< m_histoFileName);
      return StatusCode::FAILURE;
    }
  }
    
  ATH_CHECK( m_vartoolE.retrieve() ); 
  ATH_CHECK( m_vartoolEta.retrieve() );

  return StatusCode::SUCCESS;
}

StatusCode EtaJESCalibStep::calibrate(xAOD::JetContainer& jets) const {
  ATH_MSG_DEBUG("Calibrating jet collection with EtaJES.");


  const xAOD::JetAttributeAccessor::AccessorWrapper<xAOD::JetFourMom_t> jesScaleMomAcc(m_jetOutScale); 

  for(xAOD::Jet* jet: jets){
      
    const xAOD::JetFourMom_t jetStartP4 = jet->getAttribute<xAOD::JetFourMom_t>(m_jetInScale);
    jet->setJetP4(jetStartP4);
      
    // Extract the maximum energy, and store in the context
    JetHelper::JetContext jc;
    double varE {m_vartoolE->getValue(*jet,jc)};
    double varEta {m_vartoolEta->getValue(*jet,jc)};
    double Emax = 14000;
    if(m_freezeJESatHighE){
      Emax = getEmaxJES(varEta);
    }

    // Extract JES from the text handling tool
    double jesCorrection = getJES(varE, varEta, Emax);
    int ieta=getEtaBin(varEta);


    xAOD::JetFourMom_t calibP4 = jetStartP4 * jesCorrection;


    const float etaCorr = calibP4.eta() + getEtaCorr(jesCorrection*varE, varEta) ; //m_textTool_Eta->getValue(*jet, jc);
    ATH_MSG_DEBUG("eta = "<<etaCorr);

    // Apply the eta correction, use TLV from ROOT to do some math for us
    TLorentzVector TLVjet;
    TLVjet.SetPtEtaPhiM( calibP4.P()/cosh(etaCorr), etaCorr, calibP4.phi(), calibP4.M() );
    calibP4.SetPxPyPzE( TLVjet.Px(), TLVjet.Py(), TLVjet.Pz(), TLVjet.E() );
    ATH_MSG_DEBUG("JES = "<<jesCorrection << "  e="<<varE << "  eta="<<jetStartP4.Eta()<< " ieta="<<  ieta << "    post_pt= ="<< calibP4.Pt() <<  " etaCorr="<< etaCorr);

    // Set the decorations of this scale
    jesScaleMomAcc.setAttribute(*jet, calibP4);
    jet->setJetP4(calibP4);
  }

  return StatusCode::SUCCESS;   
}


bool EtaJESCalibStep::readMCJESFromText()
{
  // Open the input file
  std::string local_path=static_cast<std::string> (m_constantFileName);
  std::string fileName = PathResolverFindCalibFile(m_constantFileName);
  if(fileName=="") return false;
    
  TEnv config(fileName.c_str());

  std::string jetAlgo=static_cast<std::string> (m_jetAlgo);

  std::vector<double> etaBins = static_cast<std::vector<double>> (m_etaBins);
  if (etaBins.size()==0){ // default binning
    for (int i=0;i<=90; i++)
      etaBins.push_back(0.1*i-4.5);
  }

  ATH_MSG_DEBUG("Number eta bins: " << etaBins.size());

  m_etaBinAxis = new TAxis(etaBins.size()-1,&etaBins[0]);// this is to search for eta bin

  for (uint ieta=0; ieta<etaBins.size()-1; ++ieta)
    {
      TString key=Form("JES.%s_Bin%d",jetAlgo.c_str(),ieta);
      ATH_MSG_VERBOSE("reading: " << key << " = "<< config.GetValue(key,""));
      std::vector<double> params = VectorizeD(config.GetValue(key,"")," ");
      m_nPar = params.size();	
      ATH_MSG_VERBOSE("Number of parameters: " << m_nPar);
      for (uint ipar=0;ipar<m_nPar;++ipar) m_JESFactors[ieta][ipar] = params[ipar];
      if(m_lowPtExtrap > 0) {
	//Calculate the slope of the response curve at the minPt for each eta bin
	//Used in the GetLowPtJES method when Pt < minPt
	const double *factors = m_JESFactors[ieta];
	double Ecutoff = m_minPt_JES*cosh(etaBins[ieta]);
	if(m_useSecondaryminPt_JES && std::abs(etaBins[ieta]) >= m_etaSecondaryminPt_JES) Ecutoff = m_secondaryminPt_JES*cosh(etaBins[ieta]);
	const double Rcutoff = getLogPolN(factors,Ecutoff);
	const double Slope = getLogPolNSlope(factors,Ecutoff);
	if(Slope > Rcutoff/Ecutoff) ATH_MSG_FATAL("Slope of calibration curve at minimum ET is too steep for the JES factors of etabin " << ieta << ", eta = " << etaBins[ieta] );

	m_JES_MinPt_E[ieta] = Ecutoff;
	m_JES_MinPt_R[ieta] = Rcutoff;
	m_JES_MinPt_Slopes[ieta] = Slope;

	//Calculate the parameters for a 2nd order polynomial extension to the calibration curve below minimum ET
	//Used in the GetLowPtJES method when Pt < minPt
	if(m_lowPtExtrap == 2) {
	  ATH_MSG_ERROR("LowPtJESExtrapolationMethod==2 not supported yet");
	  return false;
	}
      }

      key=Form("EtaCorr.%s_Bin%d",jetAlgo.c_str(),ieta);
      ATH_MSG_VERBOSE("reading: " << key << " = "<< config.GetValue(key,""));
      params = VectorizeD(config.GetValue(key,"")," ");
      m_nPar = params.size();	
      ATH_MSG_VERBOSE("Number of parameters: " << m_nPar);            
      for (uint ipar=0;ipar<m_nPar;++ipar) m_etaCorrFactors[ieta][ipar] = params[ipar];

      if(m_freezeJESatHighE){
	key=Form("EmaxJES.%s_Bin%d",jetAlgo.c_str(),ieta);
	ATH_MSG_VERBOSE("reading: " << key << " = "<< config.GetValue(key,""));
	params = VectorizeD(config.GetValue(key,"")," ");
	m_energyFreezeJES[ieta] = params[0];
      }
    }
  return true;
}



bool EtaJESCalibStep::readMCJESFromHists()
{ 
  // Open the input file
  std::string local_path=static_cast<std::string> (m_constantFileName);
  std::string fileName = PathResolverFindCalibFile(m_constantFileName);

  TEnv config(fileName.c_str());
  if(fileName=="") return false;

  std::string jetAlgo=static_cast<std::string> (m_jetAlgo);

  std::vector<double> etaBins = static_cast<std::vector<double>> (m_etaBins);
  if (etaBins.size()==0){ // default binning
    for (int i=0;i<=90; i++) 
      etaBins.push_back(0.1*i-4.5);
  }

  ATH_MSG_DEBUG("Number eta bins: " << etaBins.size());

  m_etaBinAxis = new TAxis(etaBins.size()-1,&etaBins[0]);// this is to search for eta bin

  std::string calibHistFile = PathResolverFindCalibFile(m_histoFileName);
  loadSplineHists(calibHistFile, "etaJes");

  //Protections for high order extrapolation methods at low Et (Et < _minPt_JES)
  if(m_lowPtExtrap != 1) {
    ATH_MSG_ERROR("Only linear extrapolations are supported for p-splines currently. Please change the config file to reflect this");
    return false;
  }

  for (uint ieta=0;ieta<etaBins.size()-1;++ieta) {
    //Calculate the slope of the response curve at the minPt for each eta bin
    //Used in the GetLowPtJES method when Pt < minPt
    double Ecutoff= m_minPt_JES*cosh(etaBins[ieta]);
    if(m_useSecondaryminPt_JES && std::abs(etaBins[ieta]) >= m_etaSecondaryminPt_JES) Ecutoff = m_secondaryminPt_JES*cosh(etaBins[ieta]);
    const double Rcutoff = getSplineCorr(ieta, Ecutoff);
    const double Slope = getSplineSlope(ieta, Ecutoff);
    if(Slope > Rcutoff/Ecutoff) ATH_MSG_WARNING("Slope of calibration curve at minimum ET is too steep for the JES factors of etabin " << ieta << ", eta = " << etaBins[ieta] );
      
    m_JES_MinPt_E[ieta] = Ecutoff;
    m_JES_MinPt_R[ieta] = Rcutoff;
    m_JES_MinPt_Slopes[ieta] = Slope;

    TString key=Form("EtaCorr.%s_Bin%d",jetAlgo.c_str(),ieta);
    ATH_MSG_VERBOSE("reading: " << key << " = "<< config.GetValue(key,""));
    std::vector<double> params = VectorizeD(config.GetValue(key,"")," ");
    m_nPar = params.size();	
    ATH_MSG_VERBOSE("Number of parameters: " << m_nPar);            
    for (uint ipar=0;ipar<m_nPar;++ipar) m_etaCorrFactors[ieta][ipar] = params[ipar];

    if(m_freezeJESatHighE){
      key=Form("EmaxJES.%s_Bin%d",jetAlgo.c_str(),ieta);
      ATH_MSG_VERBOSE("reading: " << key << " = "<< config.GetValue(key,""));
      params = VectorizeD(config.GetValue(key,"")," ");
      m_energyFreezeJES[ieta] = params[0];
    }
    
  }
  return true;
}


double EtaJESCalibStep::getJES(const double X, const double Y, const double Emax) const
{

  if(!m_useSecondaryminPt_JES){
    if ( X/cosh(Y) < m_minPt_JES ) { // WARNING !! Won't work if X is actually pT
      double R = getLowPtJES(X,Y);
      return 1.0/R;
    }
  }
  else{
    if(std::abs(Y) < m_etaSecondaryminPt_JES && X/cosh(Y) < m_minPt_JES){
      double R = getLowPtJES(X,Y);
      return 1.0/R;
    }
    if(std::abs(Y) >= m_etaSecondaryminPt_JES && X/cosh(Y) < m_secondaryminPt_JES){
      double R = getLowPtJES(X,Y);
      return 1.0/R;
    }
  }
  
  double JES_R;
  int binEta = getEtaBin(Y);
  const double *factors = m_JESFactors[binEta];

  
  double E = X;
  if( m_freezeJESatHighE && (E>Emax) && (Emax!=-1)) {
    E = Emax;
  }

  double R = 1.;

  if(m_useSpline){
    R = getSplineCorr(binEta, E);
    return 1.0/R;
  } else { 
    R = getLogPolN(factors,E);
  }
  
  JES_R = 1/R;
  
  return JES_R;
}


double EtaJESCalibStep::getLowPtJES(double E_uncorr, double eta_det) const {
  int ieta = getEtaBin(eta_det);
  double R=1;
  // This correspond to  m_lowPtExtrap == 0 in the old EtaJESCorrection tool. Not supporting other cases yet.
  if (m_lowPtExtrap == 0) {
    const double *factors = m_JESFactors[ieta];
    double E = m_minPt_JES*cosh(eta_det);
    R= getLogPolN(factors,E);
  } else if (m_lowPtExtrap == 1) {
    double Ecutoff = m_JES_MinPt_E[ieta];
    double Rcutoff = m_JES_MinPt_R[ieta];
    double slope = m_JES_MinPt_Slopes[ieta];
    R = slope*(E_uncorr-Ecutoff)+Rcutoff;
  }
  else ATH_MSG_WARNING("Incorrect specification of low Pt JES extrapolation, please check the value of the LowPtJESExtrapolationMethod config flag.");

  return R;
}


double EtaJESCalibStep::getEtaCorr( double X,  double Y) const
{
  int binEta = getEtaBin(Y);
  const double *factors = m_etaCorrFactors[binEta];

  if ( X < m_minPt_EtaCorr*cosh(Y) ) 
    X = m_minPt_EtaCorr*cosh(Y);
  if ( X>m_maxE_EtaCorr ) X=m_maxE_EtaCorr;
    
  double eta_corr = getLogPolN(factors,X);

  return -eta_corr;
}

double EtaJESCalibStep::getEmaxJES(const double Y) const
{
  int binEta = getEtaBin(Y);
  double emaxJES = m_energyFreezeJES[binEta];

  return emaxJES;
}

double EtaJESCalibStep::getLogPolN(const double *factors, double x) const
{
  double y=0;
  for ( uint i=0; i<m_nPar; ++i )
    y += factors[i]*TMath::Power(log(x),Int_t(i));
  return y;
}



int EtaJESCalibStep::getEtaBin(double eta_det) const 
{
  int bin = std::as_const(m_etaBinAxis)->FindBin(eta_det);
  if (bin<=0) return 0;
  if (bin>m_etaBinAxis->GetNbins()) return bin-2; // overflow
  return bin-1;
}

double EtaJESCalibStep::getLogPolNSlope(const double *factors, double x) const {
  double y=0;
  const double inv_x = 1. / x;
  for ( uint i=0; i<m_nPar; ++i )
    y += i*factors[i]*TMath::Power(log(x),Int_t(i-1))*inv_x;
  return y;
}


void EtaJESCalibStep::loadSplineHists(const std::string & fileName, const std::string &etajes_name) 
{
  std::unique_ptr<TFile> tmpF(TFile::Open( fileName.c_str() ));
  TList *etajes_l = static_cast<TList*>( tmpF->Get(etajes_name.c_str()));

  m_etajesFactors.resize( etajes_l->GetSize() );
  if(etajes_l->GetSize() != m_etaBinAxis->GetNbins()+1){
    ATH_MSG_WARNING("Do not have the correct number of eta bins for " << fileName << "\t" << etajes_name << "\t" << etajes_l->GetSize() );
  }

  for(int i=0 ; i<m_etaBinAxis->GetNbins(); i++){
    auto *pTH1 = dynamic_cast<TH1*>(etajes_l->At(i));
    if (not pTH1) continue;
    m_etajesFactors[i].reset(pTH1);
    m_etajesFactors[i]->SetDirectory(nullptr);
  }
  tmpF->Close();
}


double EtaJESCalibStep::getSplineSlope(const int ieta, const double minE) const {
  // Don't want to use interpolation here, so instead just use the values at the bin centers near the cutoff
  int minBin = m_etajesFactors[ieta]->FindBin(minE);

  double rFirst = m_etajesFactors[ ieta ]->GetBinContent(minBin);
  double rSecond = m_etajesFactors[ ieta ]->GetBinContent(minBin+1);
  double binWidth = m_etajesFactors[ ieta ]->GetBinCenter(minBin+1) - m_etajesFactors[ ieta ]->GetBinCenter(minBin);
  double slope = (rSecond - rFirst) / binWidth;

  return slope;
}


double EtaJESCalibStep::getSplineCorr(const int etaBin, double E) const {
  double R = m_etajesFactors[ etaBin ]->Interpolate(E);
  return R;
}
