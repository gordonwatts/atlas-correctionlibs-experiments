/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "JetCalibTools/CalibrationMethods/Generic4VecCorrection.h"
#include "PathResolver/PathResolver.h"
#include "JetCalibTools/RootHelpers.h"

#include "TH2.h"
#include <TEnv.h>
#include "TFile.h"
#include "TObjString.h"
#include <algorithm>
#include <fstream>

Generic4VecCorrection::Generic4VecCorrection(const std::string& name, TEnv* config, 
   const TString &jetAlgo, const TString &calibAreaTag, const TString & forceCalibFile, 
   JET_CORRTYPE correctionType, const TString & mcCampaign, const TString & simFlavour, int mcDSID, 
   const TString &generatorsInfo)
  : JetCalibrationStep::JetCalibrationStep(name.c_str()),
  m_config(config), m_jetAlgo(jetAlgo), m_calibAreaTag(calibAreaTag), m_correctionType(correctionType),
  m_simFlavour(simFlavour), m_mcDSID(mcDSID), m_generatorsInfo(generatorsInfo), m_mcCampaign(mcCampaign), m_forceCalibFile(forceCalibFile), m_skipCorrection(false), m_correctionFilePath("")
{ }

Generic4VecCorrection::Generic4VecCorrection()
  : JetCalibrationStep::JetCalibrationStep(),
  m_config(nullptr), m_jetAlgo(""), m_calibAreaTag(""), m_correctionType(JET_CORRTYPE::UNKNOWN),
  m_simFlavour(""), m_mcDSID(0), m_generatorsInfo(""), m_mcCampaign(""), m_forceCalibFile(""), m_skipCorrection(true), m_correctionFilePath("")
{ }

Generic4VecCorrection::~Generic4VecCorrection()
{ }

StatusCode Generic4VecCorrection::initialize()
{
  ATH_MSG_INFO("Initializing Generic4VecCorrection correction tool.");

  if (!m_config){
      ATH_MSG_FATAL("Config file not specified.  Aborting.");
      return StatusCode::FAILURE;
  }

  // Set correction-specific information & retrieve response histogram(s)
  std::string algo_type, default_OutJetScale;
  if(m_correctionType == JET_CORRTYPE::PTRESIDUAL){
    algo_type = "JPS_PtResidual";
    default_OutJetScale = "JetPtResidualScaleMomentum";
    ATH_CHECK( initialize_correctionResponse() );
    // Save etaAxis to assist in avoiding eta-interpolation
    m_etaAxis = *(m_only_correction_2D->GetYaxis());

  } else if(m_correctionType == JET_CORRTYPE::MC2MC){
    algo_type = "JPS_MC2MC";
    default_OutJetScale = "JetMC2MCScaleMomentum";
    bool isMC = TString(m_simFlavour).Contains("FullG4",TString::kIgnoreCase) || TString(m_simFlavour).Contains("ATLFAST3",TString::kIgnoreCase);
    if (m_simFlavour == ""){
      ATH_MSG_WARNING("No simFlavour metadata available for this sample! Assuming it is MC, but this could cause an error if your sample is not listed in MC2MC_exceptions_DSID.json");
      isMC = true;
    }
    if(!isMC){
      if( m_forceCalibFile=="" ){
        ATH_MSG_INFO("Will not apply JPS_MC2MC to this Data file.");
        m_skipCorrection = true;
        return StatusCode::SUCCESS;
      } else {
        ATH_MSG_WARNING("Metadata does not indicate this is MC (assuming data), but ForceCalibFile is set so will apply JPS_MC2MC correction anyway.");
      }
    }
    ATH_CHECK( initialize_MC2MC() );

  } else if(m_correctionType == JET_CORRTYPE::FASTSIM){
    algo_type = "JPS_FastSim";
    default_OutJetScale = "JetFastSimScaleMomentum";

    bool isAF3 = TString(m_simFlavour).Contains("ATLFAST3",TString::kIgnoreCase);
    if(!isAF3){
      if( m_forceCalibFile==""){
        ATH_MSG_INFO("Will not apply JPS_FastSim to this Data or FullSim file.");
        m_skipCorrection = true;
        return StatusCode::SUCCESS;
      } else {
        ATH_MSG_WARNING("ForceCalibFile is set for a data or FullSim file, will apply JPS_FastSim correction anyway.");
      }
    }
    ATH_CHECK( initialize_correctionResponse() );

  } else{
    ATH_MSG_FATAL("Generic4VecCorrection is incorrectly configured. Aborting.");
    return StatusCode::FAILURE;
  }

  // Get the starting and ending jet scales
  m_inJetScale  = m_config->GetValue( (algo_type+".InJetScale").c_str(),  "Default");
  m_outJetScale = m_config->GetValue( (algo_type+".OutJetScale").c_str(), default_OutJetScale.c_str());
  if ( m_inJetScale != "Default" || m_outJetScale != default_OutJetScale ){
    ATH_MSG_WARNING(algo_type << " is configured to use custom jet scale input " << m_inJetScale << "and/or custom jet scale output " << m_outJetScale << ", this is expert-level only!");
  }

  ATH_MSG_INFO("Starting " << algo_type << " correction from jet scale " << m_inJetScale << " and writing out jet scale " << m_outJetScale << ", using input file " << m_correctionFilePath);
  return StatusCode::SUCCESS;
}

// Get correctionFactor from requested histogram
StatusCode Generic4VecCorrection::readHisto(float& correctionFactor, TH2* h_correction_2D, float x, float y) const
{
  // If we are outside histogram boundaries, silently take value of closest bin
  const float minX = h_correction_2D->GetXaxis()->GetBinLowEdge(1);
  const float maxX = h_correction_2D->GetXaxis()->GetBinLowEdge(h_correction_2D->GetNbinsX()+1);
  const float minY = h_correction_2D->GetYaxis()->GetBinLowEdge(1);
  const float maxY = h_correction_2D->GetYaxis()->GetBinLowEdge(h_correction_2D->GetNbinsY()+1);
  if ( x >= maxX )
      x = maxX - 1.e-6;
  else if ( x <= minX )
      x = minX + 1.e-6;
  if ( y >= maxY )
      y = maxY - 1.e-6;
  else if ( y <= minY )
      y = minY + 1.e-6;

  // Interpolate final correctionFactor
  correctionFactor = h_correction_2D->Interpolate(x,y);

  return StatusCode::SUCCESS;
}

// Perform 4Vec correction for a jet
StatusCode Generic4VecCorrection::calibrate(xAOD::Jet& jet, JetEventInfo& jetEventInfo) const
{
  (void)jetEventInfo; //Unused

  // Skip correction if we've determined it is not to be applied to this input file
  if(m_skipCorrection)
    return StatusCode::SUCCESS;

  xAOD::JetFourMom_t calibP4;
  if (m_inJetScale == "Default")
    calibP4 = jet.jetP4();
  else
    calibP4 = jet.jetP4(m_inJetScale);

  float correctionFactor = 1.0;

  TH2* h_correction_2D = nullptr;
  float this_pt, this_eta;
  if (m_correctionType == JET_CORRTYPE::PTRESIDUAL){
    this_pt = jet.pt()/1000.;
    static const SG::ConstAccessor<float> DetectorEtaAcc ("DetectorEta");
    this_eta = DetectorEtaAcc(jet);
    h_correction_2D = m_only_correction_2D;

    // PtResidual should not interpolate across eta bins, so set this_eta to the center of its histogram bin
    int eta_bin = m_etaAxis.FindBin(this_eta);
    this_eta = m_etaAxis.GetBinCenter(eta_bin);

  } else if (m_correctionType == JET_CORRTYPE::FASTSIM){
    this_pt = jet.pt()/1000.;
    this_eta = fabs(jet.rapidity());
    h_correction_2D = m_only_correction_2D;
  } else if (m_correctionType == JET_CORRTYPE::MC2MC){
    this_pt = jet.pt()/1000.;
    this_eta = fabs(jet.rapidity());

    static const SG::ConstAccessor<int> PartonTruthLabelIDAcc ("PartonTruthLabelID");
    if(!PartonTruthLabelIDAcc.isAvailable(jet))
      return StatusCode::SUCCESS;
    int jet_PID = abs(PartonTruthLabelIDAcc(jet));

    // If this jet_PID is in the map, get its h_correction_2D and find the correctionFactor
    auto correction_from_map = m_correctionHists.find(jet_PID);
    if (correction_from_map != m_correctionHists.end()){
      h_correction_2D = correction_from_map->second;
    }
  }

  if (h_correction_2D){
    ATH_CHECK( readHisto(correctionFactor, h_correction_2D, this_pt, this_eta) );
  }
  // Apply the correction and set it in the jet EDM
  calibP4 *= correctionFactor;
  jet.setAttribute<xAOD::JetFourMom_t>(m_outJetScale,calibP4);
  jet.setJetP4(calibP4);

  return StatusCode::SUCCESS;
}

StatusCode Generic4VecCorrection::initialize_correctionResponse()
{
  std::string algo_type, calibFilePrepend, corrHistName;
  if(m_correctionType == JET_CORRTYPE::PTRESIDUAL){
    algo_type = "JPS_PtResidual";
    calibFilePrepend = "PtResidual";
    corrHistName = "h_respMap_recoPt_DetEta";
  } else if(m_correctionType == JET_CORRTYPE::FASTSIM){
    algo_type = "JPS_FastSim";
    calibFilePrepend = "AF3";
    corrHistName = "h_respMap_recoPt_recoY";
  }

  // If CalibFile is set in tool configuration, we will force that generator correction.
  // Used if metadata is not available for this file or for expert-level tests
  TString CalibFile;
  if (m_forceCalibFile != ""){
    CalibFile = m_forceCalibFile;
    ATH_MSG_WARNING("Have forced " << algo_type << " CalibFile to be " << m_forceCalibFile);

  } else {
    // Recommended method to build the correct CalibFile
    std::string CalibFileTag = m_config->GetValue( (algo_type+".CalibFileTag").c_str(), ""); 
    if( CalibFileTag == "" || m_jetAlgo == "" || m_mcCampaign == ""){
      ATH_MSG_FATAL("At least one of the required parameters is not set, please check m_mcCampaign (" << m_mcCampaign << "), m_jetAlgo (" << m_jetAlgo << "), and " << algo_type << ".CalibFileTag (" << CalibFileTag <<")");
      return StatusCode::FAILURE;
    } 
    CalibFile.Append(m_calibAreaTag+"/CalibrationFactors/"+calibFilePrepend+"_"+m_mcCampaign+"_"+m_jetAlgo+"_"+CalibFileTag+".root");
  }

  m_correctionFilePath = PathResolverFindCalibFile(CalibFile.Data());
  if(m_correctionFilePath == ""){
    ATH_MSG_FATAL("PathResolverFindCalibFile cannot find path to " << CalibFile);
    return StatusCode::FAILURE;
  }
  std::unique_ptr<TFile> inputFile(TFile::Open(m_correctionFilePath.c_str()));
  if (!inputFile || inputFile->IsZombie()){
      ATH_MSG_FATAL("Cannot open " << algo_type << "'s CalibFile, even though the m_correctionFilePath exists: " << CalibFile);
      return StatusCode::FAILURE;
  }

  m_only_correction_2D = (TH2*)inputFile->Get( corrHistName.c_str() );
  if (!m_only_correction_2D){
    ATH_MSG_FATAL("Failed to retrieve histogram: " << corrHistName);
    return StatusCode::FAILURE;
  }
  m_only_correction_2D->SetDirectory(0);

  inputFile->Close();
  return StatusCode::SUCCESS;
}

StatusCode Generic4VecCorrection::load_json(nlohmann::json& json_object, const std::string& json_filepath) const
{
  std::string full_path = PathResolverFindCalibFile(json_filepath);
  std::ifstream json_stream(full_path);
  if( json_stream.peek() == std::ifstream::traits_type::eof() )
    return StatusCode::FAILURE;

  json_stream >> json_object;
  return StatusCode::SUCCESS;
}



StatusCode Generic4VecCorrection::initialize_MC2MC()
{

  std::string showerModel;
  ATH_CHECK( parse_showerModel(showerModel, m_mcDSID, m_generatorsInfo) );

  // If CalibFile is set, we will force that generator correction. This is expert-level functionality
  TString MC2MC_CalibFile;
  if (m_forceCalibFile != ""){
    MC2MC_CalibFile = m_forceCalibFile;
    ATH_MSG_WARNING("For sample of showerModel " << showerModel << ", have forced MC2MC CalibFile to be " << m_forceCalibFile);

    // Set showerModel to the requested version
    TObjArray *CalibFile_fields = MC2MC_CalibFile.Tokenize("_");
    float n_fields = CalibFile_fields->GetEntries();
    std::string new_showerModel = ((TObjString *)(CalibFile_fields->At(n_fields-1)))->String().Data();
    new_showerModel.resize(new_showerModel.find(".root")); //Remove .root
    showerModel = std::move(new_showerModel);

  } else {
    // Recommended method to build the correct CalibFile
    std::string MC2MC_CalibFileTag = m_config->GetValue("JPS_MC2MC.CalibFileTag",""); 
    if( MC2MC_CalibFileTag == "" || m_jetAlgo == "" || m_mcCampaign == "" || showerModel == ""){
      ATH_MSG_FATAL("At least one of the required parameters is not set, please check m_mcCampaign (" << m_mcCampaign << "), m_jetAlgo (" << m_jetAlgo << "), JPS_MC2MC.CalibFileTag (" << MC2MC_CalibFileTag <<"), and showerModel (" << showerModel <<")");
      return StatusCode::FAILURE;
    } 
    MC2MC_CalibFile.Append(m_calibAreaTag+"/CalibrationFactors/MC2MC_"+m_mcCampaign+"_"+m_jetAlgo+"_"+MC2MC_CalibFileTag+"_"+showerModel+".root");
  }
  // If the found / requested showerModel is the original Pythia used for calibrations, or was specifically set to None, skip the MC2MC correction
  if (showerModel.starts_with("Pythia") || showerModel.starts_with("None")){
    m_skipCorrection = true;
    ATH_MSG_INFO("Will not perform MC2MC correction for this sample (Pythia or forced to None), but will write out the redundant jet scale " << m_outJetScale);
    return StatusCode::SUCCESS;
  }
  m_correctionFilePath = PathResolverFindCalibFile(MC2MC_CalibFile.Data());
  if(m_correctionFilePath == ""){
    ATH_MSG_FATAL("PathResolverFindCalibFile cannot find path to MC2MC CalibFile: " << MC2MC_CalibFile);
    return StatusCode::FAILURE;
  }
  std::unique_ptr<TFile> inputFile(TFile::Open(m_correctionFilePath.c_str()));
  if (!inputFile || inputFile->IsZombie()){
      ATH_MSG_FATAL("Cannot open MC2MC CalibFile, even though the m_correctionFilePath exists: " << MC2MC_CalibFile);
      return StatusCode::FAILURE;
  }
  // Get list of jet PIDs to perform correction on
  std::vector<int> considered_PIDs = {1,2,3,21}; //Always correct u/d/s/g
  bool doCjetCorrection = m_config->GetValue("JPS_MC2MC.doCjetCorrection",true);
  if(doCjetCorrection)
    considered_PIDs.push_back(4);
  bool doBjetCorrection = m_config->GetValue("JPS_MC2MC.doBjetCorrection",true);
  if(doBjetCorrection)
    considered_PIDs.push_back(5);

  // Load all the requested MC2MC correction histograms into the correction map
  for (auto this_PID : considered_PIDs){
    TString this_hist_name;
    if(this_PID == 1 || this_PID == 2 || this_PID == 3){
      this_hist_name = "h_respMap_recoPt_recoY_q";
    } else if(this_PID == 4){
      this_hist_name = "h_respMap_recoPt_recoY_c";
    } else if(this_PID == 5){
      this_hist_name = "h_respMap_recoPt_recoY_b";
    } else if(this_PID == 21){
      this_hist_name = "h_respMap_recoPt_recoY_g";
    } else {
      ATH_MSG_FATAL("Requested PID " << this_PID << " is not supported for MC2MC correction, please contact JetETMiss.");
      return StatusCode::FAILURE;
    }
    TH2* this_hist = (TH2*)inputFile->Get(this_hist_name);
    if (!this_hist){
      ATH_MSG_FATAL("Failed to retrieve histogram: " << this_hist_name);
      return StatusCode::FAILURE;
    }
    this_hist->SetName( ("h_respMap_recoPt_recoY_"+std::to_string(this_PID)).c_str() );
    this_hist->SetDirectory(0);
    m_correctionHists.insert( std::make_pair(this_PID, this_hist) );
  }
  inputFile->Close();
  
  return StatusCode::SUCCESS;
}

// For MC2MC, parse showerModel from generatorsInfo, of form "Powheg(v.06-02)+Herwig7(v.7.2.3p2)+EvtGen(v.2.1.1)""
StatusCode Generic4VecCorrection::parse_showerModel(std::string& showerModel, int mcDSID, TString generatorsInfo) const
{

  // Check if an exception to the showerModel exists for this DSID
  nlohmann::json MC2MC_exceptions_DSID;
  ATH_CHECK( load_json(MC2MC_exceptions_DSID, "JetCalibTools/MC2MC_exceptions_DSID.json") );
  if( MC2MC_exceptions_DSID.contains(std::to_string(mcDSID)) ){
    showerModel = MC2MC_exceptions_DSID[std::to_string(mcDSID)];
    ATH_MSG_INFO("Sample DSID " << mcDSID << " is in the MC2MC_exceptions_DSID list, will be forcing the showerModel " << showerModel);
    return StatusCode::SUCCESS;
  }

  if(generatorsInfo == ""){
    showerModel="";
    ATH_MSG_DEBUG("No generatorsInfo string provided, cannot parse showerModel.");
    return StatusCode::SUCCESS;
  }

  // Parse shower model from MC sample generatorInfo
  TObjArray *generatorsInfo_fields = generatorsInfo.Tokenize("+");
  float n_fields = generatorsInfo_fields->GetEntries();
  std::string this_substr = ((TObjString *)(generatorsInfo_fields->At(n_fields-1)))->String().Data();

  // Remove any trailing afterburners
  while( n_fields > 0 && 
    (this_substr.starts_with("EvtGen") ||
    this_substr.starts_with("Photos") ||
    this_substr.starts_with("Tauola") ) ) {
      generatorsInfo_fields->RemoveAt(n_fields-1);
      n_fields--;

      if ( n_fields == 0 ){
        ATH_MSG_FATAL("No valid PS/Had model found in generatorsInfo string: " << generatorsInfo);
        return StatusCode::FAILURE;
      }

      this_substr = ((TObjString *)(generatorsInfo_fields->At(n_fields-1)))->String().Data();
  }
  std::string full_pshadInfo = this_substr;

  // Find generator type and set default Parton Shower / Hadronization models
  std::string genType = "";
  std::string psType = "";
  std::string hadType = "";
  std::string version = "";
  if (this_substr.starts_with("Herwigpp")){
    genType = "Herwigpp";
    psType = "angular";
    hadType = "cluster";
  } else if (this_substr.starts_with("Herwig")){
    genType = "Herwig";
    psType = "angular";
    hadType = "cluster";
  } else if (this_substr.starts_with("Sherpa")){
    genType = "Sherpa";
    psType = "dipole";
    hadType = "cluster";
  } else if (this_substr.starts_with("Pythia8B")){
    genType = "PythiaB";
    psType = "dipole";
    hadType = "cluster";
    version += "8";
  } else if (this_substr.starts_with("Pythia")){
    genType = "Pythia";
    psType = "dipole";
    hadType = "cluster";
  } else {
    ATH_MSG_FATAL("No valid generator type found in generatorsInfo string: " << generatorsInfo);
    return StatusCode::FAILURE;
  }
  
  // Parse version number of the generator
  this_substr = this_substr.substr(this_substr.find("(v.") + 3); // Remove up to first version number in e.g. Herwig7(v.7.2.3p2)
  if( full_pshadInfo.starts_with("Pythia8") && !this_substr.starts_with("8")){ // Exception for out-of-order Pythia version
    version += "8";
  }

  // Remove any trailing characters after the version number e.g. (v.7.2.3p2)
  std::vector<std::string> version_exceptions = {"alpha", "p", "bbb", "atlas", "beta", ")"};
  for (const auto& exception : version_exceptions) {
    if (this_substr.find(exception) != std::string::npos) {
      this_substr.resize(this_substr.find(exception));
    }
  }

  // Remove all periods so that only numbers remain
  this_substr.erase(std::remove(this_substr.begin(), this_substr.end(), '.'), this_substr.end());

  // What is left is the version tag
  version += this_substr;

  // Build the full showerModel tag
  showerModel = genType+"-"+version+"-"+psType+"-"+hadType;

  // Check if a remap of this showerModel version is requested, if so use it
  // We do this even for mapped_DSID, to allow for simple swapping of showerModel across all exceptions
  nlohmann::json MC2MC_showerRemap;
  ATH_CHECK( load_json(MC2MC_showerRemap, "JetCalibTools/MC2MC_showerRemap.json") );

  if( MC2MC_showerRemap.contains( genType+"-"+version+"-"+psType+"-"+hadType ) ){
    std::string replaced_showerModel = MC2MC_showerRemap[ genType+"-"+version+"-"+psType+"-"+hadType ];
    ATH_MSG_INFO("Sample with identified showerModel " << genType+"-"+version+"-"+psType+"-"+hadType << " is in the MC2MC_showerRemap list, will be forcing the showerModel " << replaced_showerModel);
    showerModel = std::move(replaced_showerModel);
  } else if (MC2MC_showerRemap.contains( genType+"-"+version ) ){ 
    std::string replaced_showerModel = MC2MC_showerRemap[ genType+"-"+version ];
    replaced_showerModel += "-"+psType+"-"+hadType;
    ATH_MSG_INFO("Sample with identified showerModel " << genType+"-"+version+"-"+psType+"-"+hadType << " is in the MC2MC_showerRemap list, will be forcing the showerModel " << replaced_showerModel);
    showerModel = std::move(replaced_showerModel);
  }
  
  return StatusCode::SUCCESS;
}
