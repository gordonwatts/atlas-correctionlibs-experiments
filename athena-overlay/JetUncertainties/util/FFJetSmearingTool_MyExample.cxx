/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                  //
// Name: FFJetSmearingTool_MyExample                                                                                                //
// Purpose: example code for using the FFJetSmearingTool                                                                            //
// ---> For more information about the tool, check TWiki: https://twiki.cern.ch/twiki/bin/viewauth/AtlasProtected/FFJetSmearingTool //
//                                                                                                                                  //
// * Authors: Alberto Prades and Davide Melini                                                                                      //
// * Updates: Daniel Camarero Munoz                                                                                                 //
//    * July 2024: Migration to R22+ for Phase-I large-R UFO pre-recommendations                                                    //
//    * November 2024: Available in Athena by default                                                                               //
//                                                                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// System
#include <memory>
#include <AsgTools/ToolHandle.h>
#include <AsgTools/AsgTool.h>
#include "xAODRootAccess/Event.h"

// xAOD EDM classes
#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>

#include <xAODEventShape/EventShape.h>
#include <xAODRootAccess/tools/Message.h>
#include <xAODRootAccess/tools/ReturnCheck.h>
#include <xAODCore/tools/IOStats.h>
#include <xAODCore/tools/ReadStats.h>
#include <xAODCore/ShallowCopy.h>

// Jet tools
#include <JetCalibTools/JetCalibrationTool.h>
#include <ParticleJetTools/JetTruthLabelingTool.h>
#include <JetUncertainties/FFJetSmearingTool.h>

// ROOT
#include <TFile.h>
#include <TCanvas.h>
#include <TH1F.h>
#include <TTree.h>

// Error checking macro
#define CHECK( ARG )                                          \
  do {                                                        \
    const bool result = ARG;                                  \
    if (!result){                                             \
      std::cout << "Failed to execute: " << ARG << std::endl; \
      return 1;                                               \
    }                                                         \
  } while( false )

// Help message if the --help option is given by the user
void usage(){
  std::cout << "Running options:" << std::endl;
  std::cout << "YOU HAVE TO ADAPT THE OPTIONS TO FFJETSMEARINGCORRECTION" << std::endl;
  std::cout << "        --help : To get the help you're reading" << std::endl;
  std::cout << "        --jetColl= : Specify the jet collection (e.g. UFO+CSSK jets)" << std::endl;
  std::cout << "        --MassDef= : Specify the kind of jet mass used (e.g. UFO)" << std::endl;
  std::cout << "        --MCType= : Specify the MC campaign (e.g. MC20, MC21, MC23, MC20AF3, MC23AF3)" << std::endl;
  std::cout << "        --sample= : Specify input xAOD" << std::endl;
  std::cout << "        Example: FFJetSmearingTool_MyExample --truth_jetColl=AntiKt10TruthSoftDropBeta100Zcut10Jets --reco_jetColl=AntiKt10UFOCSSKSoftDropBeta100Zcut10Jets --MassDef=UFO --MCType=MC23 --eventsMax=30000 --sample=/eos/atlas/atlascerngroupdisk/perf-jets/INSITU/TestFiles/dcamarer/DAOD_PHYS_MC_Zqqjets/mc20_13TeV/DAOD_PHYS.40038344._000011.pool.root.1 --output=.root --ConfigFile=/afs/cern.ch/user/d/dcamarer/private/PostDoc/JETM/JMRcombination_PhaseILargeR/test-jetcalibtools-configs/R22/source/JetUncertainties/share/DCM_240502_new/R10_FullJMR_Phase1.config --DebugTool=false" << std::endl;
}

///////////////////
// Main Function //
///////////////////

int main(int argc, char* argv[]){

  ////// Reading user instructions of the example

  //// Declaring input variables with default values
  std::string sample = "";
  std::string truth_jetColl = "";
  std::string reco_jetColl = "";
  std::string string_kindofmass = "";
  std::string string_kindofmc = "";
  std::string output = "";
  std::string string_configfile_path = "";
  std::string string_debugtool = "";
  int eventsMax = 30000;

  //// Decoding the user settings
  for (int i=1; i< argc; i++){

    std::string opt(argv[i]); std::vector< std::string > v;
    std::istringstream iss(opt);
    std::string item;
    char delim = '=';

    while (std::getline(iss, item, delim)){
      v.push_back(item);
    }

    if ( opt.find("--help") != std::string::npos ){
      usage();
      return 0;
    }

    if ( opt.find("--sample=")        != std::string::npos ) sample = v[1];
    if ( opt.find("--truth_jetColl=") != std::string::npos ) truth_jetColl = v[1];
    if ( opt.find("--reco_jetColl=")  != std::string::npos ) reco_jetColl = v[1];
    if ( opt.find("--MassDef=")       != std::string::npos ) string_kindofmass = v[1];
    if ( opt.find("--MCType=")        != std::string::npos ) string_kindofmc = v[1];
    if ( opt.find("--ConfigFile=")    != std::string::npos ) string_configfile_path = v[1];
    if ( opt.find("--DebugTool=")     != std::string::npos ) string_debugtool = v[1];
    if ( opt.find("--output=")        != std::string::npos ) output = v[1];
    if ( opt.find("--eventsMax=")     != std::string::npos ) eventsMax = std::atoi(v[1].data());

  }

  if (sample==""){
    std::cout << "No input xAOD file specified, exiting" << std::endl;
    return 1;
  }
  if (truth_jetColl==""){
    std::cout << "No truth jet collection specified, exiting" << std::endl;
    return 1;
  }
  if (reco_jetColl==""){
    std::cout << "No truth jet collection specified, exiting" << std::endl;
    return 1;
  }
  if (string_kindofmass==""){
    std::cout << "No kind of jet mass specified, exiting" << std::endl;
    return 1;
  }
  if (string_kindofmc==""){
    std::cout << "No kind of MC specified, exiting" << std::endl;
    return 1;
  }
  if (string_configfile_path==""){
    std::cout << "No ConfigFile specified, exiting" << std::endl;
    return 1;
  }
  if (string_debugtool==""){
    std::cout << "No debugtool specified, exiting" << std::endl;
    return 1;
  }
  if (output==""){
    std::cout << "Output not specified, exiting" << std::endl;
    return 1;
  }
  std::cout << sample << truth_jetColl << reco_jetColl << output << string_debugtool << std::endl;

  TString kindofmass = string_kindofmass;
  TString kindofmc   = string_kindofmc;

  bool want_to_debug = false;
  if (string_debugtool == "true"){
    want_to_debug = true;
  }else if (string_debugtool == "false"){
    want_to_debug = false;
  }

  ////// Opening input file

  std::unique_ptr< TFile > ifile( TFile::Open( sample.c_str(), "READ" ) );

  // Create a Event object
  auto event = xAOD::Event::createAndReadFrom(*ifile);
  if (!event) {
    std::cout << "Failed to open file " << std::endl;
    return 1;
  }

  ////// Initialization of FFJetSmearingTool

  const std::string name_FFJetSmearingTool = "FFJetSmearing_Example";
  CP::FFJetSmearingTool ffjetsmearingtool(name_FFJetSmearingTool.c_str());
  CHECK(ffjetsmearingtool.setProperty("MassDef", kindofmass));
  CHECK(ffjetsmearingtool.setProperty("MCType", kindofmc));
  CHECK(ffjetsmearingtool.setProperty("ConfigFile", string_configfile_path));
  CHECK(ffjetsmearingtool.setProperty("OutputLevel", MSG::ERROR)); // Set output level threshold (1=VERBOSE, 2=DEBUG, 3=INFO, 4=WARNING, 5=ERROR, 6=FATAL, 7=ALWAYS)
  if (want_to_debug){
    CHECK(ffjetsmearingtool.setProperty("OutputLevel", MSG::VERBOSE));
  }
  CHECK(ffjetsmearingtool.initialize());

  ////// Print the recommended systematics

  const CP::SystematicSet& recommendedSysts = ffjetsmearingtool.recommendedSystematics();
  std::cout << "\nRecommended systematics:\n" << std::endl;
  for (auto sysItr = recommendedSysts.begin(); sysItr != recommendedSysts.end(); ++sysItr){
    std::cout << sysItr->name().c_str() << std::endl;
  }

  std::vector<CP::SystematicSet> sysList;

  ////// Calibrate jets

  const std::string name_JetCalibTools = "JetCalibrationTool";
  JetCalibrationTool m_JetCalibrationTool_handle(name_JetCalibTools.c_str());

  CHECK(m_JetCalibrationTool_handle.setProperty("JetCollection", "AntiKt10UFOCSSKSoftDropBeta100Zcut10"));
  CHECK(m_JetCalibrationTool_handle.setProperty("ConfigFile", "JES_MC20PreRecommendation_R10_UFO_CSSK_SoftDrop_JMS_R21Insitu_26Nov2024.config"));
  //CHECK(m_JetCalibrationTool_handle.setProperty("ConfigFile", "share/JES_MC20PreRecommendation_R10_UFO_CSSK_SoftDrop_JMS_R21Insitu_02Aug2024.config")); // When using DEVmode
  CHECK(m_JetCalibrationTool_handle.setProperty("CalibSequence", "EtaJES_JMS"));
  CHECK(m_JetCalibrationTool_handle.setProperty("IsData", false));
  CHECK(m_JetCalibrationTool_handle.setProperty("CalibArea", "00-04-82"));
  CHECK(m_JetCalibrationTool_handle.setProperty("DEVmode", false));
  //CHECK(m_JetCalibrationTool_handle.setProperty("DEVmode", true)); // When using DEVmode
  CHECK(m_JetCalibrationTool_handle.initialize());

  ////// Check systematic uncertainties

  ////// Generate nominal systematic: in principle not needed as we save the precalibrated values.
  /*
    sysList.insert( sysList.begin(), CP::SystematicSet() );
    const CP::SystematicVariation nullVar = CP::SystematicVariation("Nominal");
    sysList.back().insert(nullVar);
  */

  for (auto sys : recommendedSysts){
    sysList.push_back(CP::SystematicSet({sys}));
  }

  std::cout << "\n=============SYSTEMATICS CHECK NOW" << std::endl;
  for (auto sys : sysList){

    std::cout << "\nRunning over the systematic " << sys.name().c_str() << std::endl;
    static constexpr float MeVtoGeV = 1.e-3;

    // Tell the calibration tool which variation to apply
    if (ffjetsmearingtool.applySystematicVariation(sys) != StatusCode::SUCCESS){
      std::cout << "Error, Cannot configure calibration tool for systematics" << std::endl;
    }

    // Define histograms
    TH1F* reco_jet_mass_hist = new TH1F("reco_jet_mass_hist","reco_jet_mass_hist", 300, 0, 600);
    TH1F* matched_truth_jet_mass_hist = new TH1F("matched_truth_jet_mass_hist","matched_truth_jet_mass_hist", 300, 0, 600);
    TH1F* smeared_reco_jet_mass_hist = new TH1F("smeared_reco_jet_mass_hist","smeared_reco_jet_mass_hist", 300, 0, 600);
    //
    TH1F* reco_jet_pt_hist = new TH1F("reco_jet_pt_hist","reco_jet_pt_hist", 800, 0, 4000);
    TH1F* matched_truth_jet_pt_hist = new TH1F("matched_truth_jet_pt_hist","matched_truth_jet_pt_hist", 800, 0, 4000);
    TH1F* smeared_reco_jet_pt_hist = new TH1F("smeared_reco_jet_pt_hist","smeared_reco_jet_pt_hist", 800, 0, 4000);
    //
    TH1F* reco_jet_rapidity_hist = new TH1F("reco_jet_rapidity_hist","reco_jet_rapidity_hist", 50, -2.5, 2.5);
    TH1F* matched_truth_jet_rapidity_hist = new TH1F("matched_truth_jet_rapidity_hist","matched_truth_jet_rapidity_hist", 50, -2.5, 2.5);
    TH1F* smeared_reco_jet_rapidity_hist = new TH1F("smeared_reco_jet_rapidity_hist","smeared_reco_jet_rapidity_hist", 50, -2.5, 2.5);

    reco_jet_mass_hist->Sumw2();
    matched_truth_jet_mass_hist->Sumw2();
    smeared_reco_jet_mass_hist->Sumw2();

    reco_jet_pt_hist->Sumw2();
    matched_truth_jet_pt_hist->Sumw2();
    smeared_reco_jet_pt_hist->Sumw2();

    reco_jet_rapidity_hist->Sumw2();
    matched_truth_jet_rapidity_hist->Sumw2();
    smeared_reco_jet_rapidity_hist->Sumw2();

    int upperlimit1 = 600;
    int upperlimit2 = 1000;

    int numBinsMass = 120;
    int numBinsPt = 100;

    std::unique_ptr<TH3F> hist_jet_mass_scale_change_3D;
    hist_jet_mass_scale_change_3D = std::make_unique<TH3F>("hist_jet_mass_scale_change_3D","hist_jet_mass_scale_change_3D",numBinsPt,0,upperlimit2,numBinsMass,0,upperlimit1,numBinsMass,0,upperlimit1);

    float lowerlimit3 = -0.5;
    float upperlimit3 = 0.5;
    int numBinsDiff = 100;

    std::unique_ptr<TH3F> hist_jet_mass_resolution_change_3D;
    hist_jet_mass_resolution_change_3D = std::make_unique<TH3F>("hist_jet_mass_resolution_change_3D","hist_jet_mass_resolution_change_3D",numBinsPt,0,upperlimit2,numBinsMass,0,upperlimit1,numBinsDiff,lowerlimit3,upperlimit3);

    ////// Loop over events

    Long64_t nevents = event->getEntries();
    
    if (eventsMax < nevents){
      nevents = eventsMax;
    }

    for (Long64_t ievent = 0;  ievent < nevents; ++ievent){

      // Load the event:
      if ( event->getEntry( ievent ) < 0 ){
        std::cout << "Failed to load entry " << ievent << std::endl;
        return 1;
      }

      // Show status
      if (ievent % 1000==0){
        std::cout << "Event " << ievent << " of " << nevents << std::endl;
      }

      // Print some event information for fun
      if (want_to_debug){
        const xAOD::EventInfo* ei = nullptr;
        CHECK( event->retrieve(ei, "EventInfo") );

        std::cout << "===>>>  start processing event " << ei->eventNumber() << ", run " << ei->runNumber() << " - Events processed so far: " << ievent << std::endl;

        // Get the truth jets from the event
        const xAOD::JetContainer* jets_truth = nullptr;
        CHECK( event->retrieve(jets_truth, truth_jetColl) );
        std::cout << "Number of truth jets: " << jets_truth->size() << std::endl;

        // Loop over the truth jets in the event
        for (const xAOD::Jet* jet_truth : *jets_truth){
          // Print basic info about this jet
          std::cout << "Truth Jet: pt = " << jet_truth->pt()*MeVtoGeV << ", mass = " << jet_truth->m()*MeVtoGeV << ", eta = " << jet_truth->eta() << std::endl;
        }

        // Get the reco jets from the event
        const xAOD::JetContainer* jets_reco = nullptr;
        CHECK( event->retrieve(jets_reco, reco_jetColl) ); 
        std::cout << "Number of reco jets: " << jets_reco->size() << std::endl;

        //Loop over the reco jets in the event
        for (const xAOD::Jet* jet_reco : *jets_reco){
          // Print basic info about this jet
          std::cout << "Reco Jet: pt = " << jet_reco->pt()*MeVtoGeV << ", mass = " << jet_reco->m()*MeVtoGeV << ", eta = " << jet_reco->eta() << std::endl;
        }
      }

      xAOD::Jet jet_truth_matched;
      jet_truth_matched.makePrivateStore();

      // Retrieve jet container
      const xAOD::JetContainer* jets = nullptr;
      CHECK( event->retrieve( jets, reco_jetColl ) );

      // Shallow copy
      auto jets_shallowCopy = xAOD::shallowCopy( *jets );

      ////// Give a TruthLabel to the jets. Needed in the FFSmearingTool to apply the uncertainties of one jet topology or another.
      // Note: in principle not needed for most of the derivaiton formats, as we save this information in DAODs
      //m_JetTruthLabelingTool.modify(*(jets_shallowCopy.first));

      if (want_to_debug){
        std::cout << "Start the loop over the jets" << std::endl;
      }

      bool lead_jet = true;

      // Apply the jet calibration
      if ( m_JetCalibrationTool_handle.applyCalibration( *(jets_shallowCopy.first) ) != StatusCode::SUCCESS ){
        std::cout << "JetCalibration tool reported a EL::StatusCode::FAILURE" << std::endl;
        return 1;
      }

      ////// Iterate over the calibrated jets

      for ( xAOD::Jet* jet_reco : *(jets_shallowCopy.first) ){

        double jetpt = jet_reco->pt() * MeVtoGeV;
        double jetm = jet_reco->m() * MeVtoGeV;
        double jetrapidity = jet_reco->rapidity();
        // Jet pT cut
        if (jetpt < 200 or jetpt > 3000) continue;
        // Jet mass cut
        if (jetm > 600) continue;
        // Jet rapidity cut
        if (abs(jetrapidity) > 2) continue;

        if ( ffjetsmearingtool.getMatchedTruthJet(*jet_reco, jet_truth_matched) != StatusCode::SUCCESS ){
          continue;
        }

        double aux_original_jet_mass = jet_reco->m() * MeVtoGeV;

        if (lead_jet == true && aux_original_jet_mass > 0){

          reco_jet_mass_hist->Fill(jet_reco->m() * MeVtoGeV);
          reco_jet_pt_hist->Fill(jet_reco->pt() * MeVtoGeV);
          reco_jet_rapidity_hist->Fill(jet_reco->rapidity());
          //
          matched_truth_jet_mass_hist->Fill(jet_truth_matched.m() * MeVtoGeV);
          matched_truth_jet_pt_hist->Fill(jet_truth_matched.pt() * MeVtoGeV);
          matched_truth_jet_rapidity_hist->Fill(jet_truth_matched.rapidity());

          // Smear the jets for each systematic variation
          if ( ffjetsmearingtool.applyCorrection(*jet_reco) == CP::CorrectionCode::Error ) {
            std::cout << "FFJetSmearingTool reported a EL::StatusCode::FAILURE" << std::endl;
            return 1;
          }

          smeared_reco_jet_mass_hist->Fill(jet_reco->m() * MeVtoGeV);
          smeared_reco_jet_pt_hist->Fill(jet_reco->pt() * MeVtoGeV);
          smeared_reco_jet_rapidity_hist->Fill(jet_reco->rapidity());

          hist_jet_mass_scale_change_3D->Fill(jet_reco->pt() * MeVtoGeV, aux_original_jet_mass, jet_reco->m() * MeVtoGeV);
          hist_jet_mass_resolution_change_3D->Fill(jet_reco->pt() * MeVtoGeV, aux_original_jet_mass, TMath::Abs( (jet_reco->m()*MeVtoGeV) - (aux_original_jet_mass) )/aux_original_jet_mass);

          lead_jet = false;

        }

      } // Loop over reco jets

    } // Loop over number of events

    TString output_path = "output/" + sys.name() + output  ;
    TFile *jetmass_histograms = new TFile(output_path,"recreate");

    reco_jet_mass_hist->Write();
    matched_truth_jet_mass_hist->Write();
    smeared_reco_jet_mass_hist->Write();

    reco_jet_pt_hist->Write();
    matched_truth_jet_pt_hist->Write();
    smeared_reco_jet_pt_hist->Write();

    reco_jet_rapidity_hist->Write();
    matched_truth_jet_rapidity_hist->Write();
    smeared_reco_jet_rapidity_hist->Write();

    ////// 3D plots for testing the JetMassScale and JetMassResolution smearing for each systematic

    TH2F* hist_jet_mass_scale_change_2D = new TH2F("hist_jet_mass_scale_change_2D","hist_jet_mass_scale_change_2D",numBinsPt,0,upperlimit2,numBinsMass,0,upperlimit1);
    for (int i=1; i<=hist_jet_mass_scale_change_3D->GetNbinsX(); i++){
      for (int j=1; j<= hist_jet_mass_scale_change_3D->GetNbinsY(); j++){
        TH1F* hold = new TH1F("","",numBinsMass,0,upperlimit1);
        for (int k=1; k<= hist_jet_mass_scale_change_3D->GetNbinsZ(); k++){
          hold->SetBinContent(k,hist_jet_mass_scale_change_3D->GetBinContent(i,j,k)); // A 2D projection of the TH3
        }
        hist_jet_mass_scale_change_2D->SetBinContent(i,j,hold->GetMean()/hist_jet_mass_scale_change_3D->GetYaxis()->GetBinCenter(j));
        delete hold;
      }
    }

    TH2F* hist_jet_mass_resolution_change_2D = new TH2F("hist_jet_mass_resolution_change_2D","hist_jet_mass_resolution_change_2D",numBinsPt,0,upperlimit2,numBinsMass,0,upperlimit1);
    for (int i=1; i<=hist_jet_mass_resolution_change_3D->GetNbinsX(); i++){
      for (int j=1; j<= hist_jet_mass_resolution_change_3D->GetNbinsY(); j++){
        TH1F* hold = new TH1F("","",numBinsDiff,lowerlimit3,upperlimit3);
        for (int k=1; k<= hist_jet_mass_resolution_change_3D->GetNbinsZ(); k++){
          hold->SetBinContent(k,hist_jet_mass_resolution_change_3D->GetBinContent(i,j,k)); // A 2D projection of the TH3
        }
        hist_jet_mass_resolution_change_2D->SetBinContent(i,j,hold->GetMean()/hist_jet_mass_resolution_change_3D->GetYaxis()->GetBinCenter(j));
        delete hold;
      }
    }

    ////// Plotting factory

    double w = 650;
    double h = 650;

    TCanvas *c1 = new TCanvas("c1","c1",w,h);
    c1->SetWindowSize(w+(w-c1->GetWw()),h+(h-c1->GetWh()));
    c1->cd();

    TPad *canvas_1 = new TPad("canvas_1", "canvas_1",0.0,0.0,1.0,1.0);
    canvas_1->SetRightMargin(0.10);
    canvas_1->SetFillStyle(4000);
    canvas_1->Draw();
    canvas_1->cd();

    hist_jet_mass_scale_change_2D->GetXaxis()->SetTitleOffset(1.5);
    hist_jet_mass_scale_change_2D->GetXaxis()->SetNdivisions(5);
    hist_jet_mass_scale_change_2D->GetYaxis()->SetTitleOffset(1.5);
    hist_jet_mass_scale_change_2D->GetZaxis()->SetTitleOffset(1.5);
    hist_jet_mass_scale_change_2D->GetYaxis()->SetTitle("Initial Reco Mass [GeV]");
    hist_jet_mass_scale_change_2D->GetXaxis()->SetTitle("Reco p_{T} [GeV]");
    hist_jet_mass_scale_change_2D->GetZaxis()->SetTitle("Average Mass smearing (Initial_reco_mass/smeared_reco_mass)");
    hist_jet_mass_scale_change_2D->GetZaxis()->SetRangeUser(0.95,1.05);
    hist_jet_mass_scale_change_2D->GetZaxis()->SetLabelSize(0.035);
    hist_jet_mass_scale_change_2D->Draw("colz");
    gPad->RedrawAxis();

    TString output_path_scale_debug = "output/debug_plots/scale_variations/" + sys.name() + "_scaleDebug.pdf"  ;
    c1->Print(output_path_scale_debug);

    delete hist_jet_mass_scale_change_2D;
    delete c1;

    TCanvas *c2 = new TCanvas("c2","c2",w,h);
    c2->SetWindowSize(w+(w-c2->GetWw()),h+(h-c2->GetWh()));
    c2->cd();

    TPad *canvas_2 = new TPad("canvas_2", "canvas_2",0.0,0.0,1.0,1.0);
    canvas_2->SetRightMargin(0.10);
    canvas_2->SetFillStyle(4000);
    canvas_2->Draw();
    canvas_2->cd();

    hist_jet_mass_resolution_change_2D->GetXaxis()->SetTitleOffset(1.5);
    hist_jet_mass_resolution_change_2D->GetXaxis()->SetNdivisions(5);
    hist_jet_mass_resolution_change_2D->GetYaxis()->SetTitleOffset(1.5);
    hist_jet_mass_resolution_change_2D->GetZaxis()->SetTitleOffset(1.5);
    hist_jet_mass_resolution_change_2D->GetYaxis()->SetTitle("Initial Reco Mass [GeV]");
    hist_jet_mass_resolution_change_2D->GetXaxis()->SetTitle("Reco p_{T} [GeV]");
    hist_jet_mass_resolution_change_2D->GetZaxis()->SetTitle("Average Mass smearing (Initial_reco_mass/smeared_reco_mass)");
    hist_jet_mass_resolution_change_2D->GetZaxis()->SetRangeUser(-0.1,0.1);
    hist_jet_mass_resolution_change_2D->GetZaxis()->SetLabelSize(0.035);
    hist_jet_mass_resolution_change_2D->Draw("colz");
    gPad->RedrawAxis();

    TString output_path_resolution_debug = "output/debug_plots/resolution_variations/" + sys.name() + "_resolutionDebug.pdf"  ;
    c2->Print(output_path_resolution_debug);

    delete hist_jet_mass_resolution_change_2D;
    delete c2;

    jetmass_histograms->Close();
    delete jetmass_histograms;

    delete reco_jet_mass_hist;
    delete matched_truth_jet_mass_hist;
    delete smeared_reco_jet_mass_hist;

    delete reco_jet_pt_hist;
    delete matched_truth_jet_pt_hist;
    delete smeared_reco_jet_pt_hist;

    delete reco_jet_rapidity_hist;
    delete matched_truth_jet_rapidity_hist;
    delete smeared_reco_jet_rapidity_hist;

  } // Loop over systematic uncertainties

  return 0;

}
