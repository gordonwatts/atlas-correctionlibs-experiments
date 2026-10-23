/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Author: Davide Melini                                            //
//                                                                  //
// Updates by Daniel Camarero                                       //
//  - 15.07.2024: significant update for UFO R22 pre-recs           //
//  - 26.11.2024: significant changes when including this in Athena //
//                                                                  //
//////////////////////////////////////////////////////////////////////

#include <filesystem>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cmath>
#include <memory> //to use make_unique

#include <TROOT.h>
#include <TStyle.h>
#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TGraphErrors.h>
#include <TLorentzVector.h>
#include <TMath.h>
#include <TH1D.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TH2F.h>
#include <TH3D.h>
#include <TH3F.h>
#include <TRandom3.h>
#include <TString.h>
#include <TLatex.h>
#include <TLine.h>
#include <TPave.h>
#include <TPad.h>
#include <TMarker.h>
#include <TSystem.h>

#include <Math/Interpolator.h>
#include <JetUncertainties/JetHelpers.h>

// The function in JetHelpers can not be used because it needs a TH1 and we use TH2 histograms. We define our own function.
double Interpolate2D(const TH2* histo, double x, double y){

  if (not histo){
    std::cout << "Histogram pointer is null in FFJetSmearingTool::Interpolate2D" << std::endl;
    return 0.;
  }

  double aux_x = x;
  double aux_y = y;
  
  ///// Asymptotic values
  // If the value is outside the histogram region, we take the closest value to that one

  double xMax = histo->GetXaxis()->GetBinLowEdge(histo->GetNbinsX()+1);
  double xMin = histo->GetXaxis()->GetBinLowEdge(1);
  double yMax = histo->GetYaxis()->GetBinLowEdge(histo->GetNbinsY()+1);
  double yMin = histo->GetYaxis()->GetBinLowEdge(1);

  if (x >= xMax) aux_x = xMax-1e-6 ; //so it fits the up-most x-bin
  if (x <= xMin) aux_x = xMin+1e-6 ; //so it fits the low-most x-bin
  if (y >= yMax) aux_y = yMax-1e-6 ; //so it fits the up-most y-bin
  if (y <= yMin) aux_y = yMin+1e-6 ; //so it fits the low-most y-bin

  Int_t bin_x = histo->GetXaxis()->FindFixBin(aux_x);
  Int_t bin_y = histo->GetYaxis()->FindFixBin(aux_y);
  if (bin_x<1 || bin_x>histo->GetNbinsX() || bin_y<1 || bin_y>histo->GetNbinsY()){
    std::cout << "The point is outside the histogram domain." << std::endl;
    return 0.;
  }
  
  double interpolated_value = JetHelpers::Interpolate(histo, aux_x, aux_y);
  return interpolated_value;

}

// Read the 3D input histograms
double Read3DHistogram(const TH3* histo, double x, double y, double z){

  if (not histo){
    std::cout << "Histogram pointer is null in FFJetSmearingTool::Read3DHistogram" << std::endl;
    return 0.;
  }

  double aux_x = x;
  double aux_y = y;
  double aux_z = z;

  ///// Asymptotic values
  // If the value is outside the histogram region, we take the closest value to that one

  double xMax = histo->GetXaxis()->GetBinLowEdge(histo->GetNbinsX()+1);
  double xMin = histo->GetXaxis()->GetBinLowEdge(1);
  double yMax = histo->GetYaxis()->GetBinLowEdge(histo->GetNbinsY()+1);
  double yMin = histo->GetYaxis()->GetBinLowEdge(1);
  double zMax = histo->GetZaxis()->GetBinLowEdge(histo->GetNbinsZ()+1);
  double zMin = histo->GetZaxis()->GetBinLowEdge(1);

  if (x >= xMax) aux_x = xMax-1e-6 ; //so it fits the up-most x-bin
  if (x <= xMin) aux_x = xMin+1e-6 ; //so it fits the low-most x-bin
  if ( std::isnan(y)) return 0; // no weight if the input is NaN, can happen for log(X)
  if (y >= yMax) aux_y = yMax-1e-6 ; //so it fits the up-most y-bin
  if (y <= yMin) aux_y = yMin+1e-6 ; //so it fits the low-most y-bin
  if (z >= zMax) aux_z = zMax-1e-6 ; //so it fits the up-most z-bin
  if (z <= zMin) aux_z = zMin+1e-6 ; //so it fits the low-most z-bin

  //Use the interpolate function from JetHelpers.cxx
  double interpolated_value = JetHelpers::Interpolate(histo, aux_x, aux_y, aux_z);
  return interpolated_value;

}

// Method to create log-spaced bins (if x-axis is log)
std::vector<double> makeLogBins(int nbins, double min, double max){
  std::vector<double> bins;
  double lmin = log(min), k=(log(max)-log(min))/nbins;
  for (int i=0;i<=nbins;++i){
    bins.push_back(exp(lmin+k*i));
    //std::cout << " bins.at("<<i<<") = " << exp(lmin+k*i) << std::endl;
  }
  return bins;
}
  
void MakeJMRPlot(int atlas_approved, const std::string & histofile, const std::string & uncmodel, const std::string & kind_of_mass, const std::string & kind_of_mc, bool dointerpol, int mass_bin, double eta_bin, bool pTbin=false){

  // Make a log-binned histogram: used to read the uncertainties originally
  std::vector<double> npTbinning;
  double original_ptmin = 0, original_ptmax = 0;
  if (pTbin){
    original_ptmin = 40, original_ptmax = 600;
    npTbinning = makeLogBins(560,original_ptmin,original_ptmax);
  }else{
    original_ptmin = 200, original_ptmax = 3000;
    npTbinning = makeLogBins(2800,original_ptmin,original_ptmax);
  }
  int npTbins = -1+npTbinning.size();
  
  std::unique_ptr<TFile> f_JMR_uncertainties_histograms = std::make_unique<TFile>(histofile.c_str(), "READ");
  if (f_JMR_uncertainties_histograms == 0){
    std::cout << "Can't find input file.\n";
    return;
  }
  
  std::cout << "\n > Input root file opened \n" << std::endl;
    
  std::vector<std::string> UFO_JMR_uncertainties_histo_names = {};
  std::vector<std::string> uncertainty_names = {};
  std::vector<std::string> uncertainty_treatment = {};

  if (uncmodel == "SimpleJMR"){

    UFO_JMR_uncertainties_histo_names = {"JMRUnc_mc20vsmc21_"+kind_of_mc+"_PreRec", "JMRUnc_mc20vsmc23_"+kind_of_mc+"_PreRec", "JMRUnc_mc20FullSimvsmc20AF3_"+kind_of_mc+"_PreRec", "JMRUnc_Noise_PreRec", "JMRUnc_mc20UFOvsmc16UFO_PreRec", "JMR_FlatSmearing20_AntiKt10UFOCSSKSoftDropBeta100Zcut10"};
    
    uncertainty_names = {"MC20 vs MC21 ("+kind_of_mc+")", "MC20 vs MC23 ("+kind_of_mc+")", "FullSim vs AF3 ("+kind_of_mc+")", "Noise", "MC20 UFO vs MC16 UFO", "Flat smearing 20%"};
    
    uncertainty_treatment = {"eLOGmOeAbsEta", "eLOGmOeAbsEta", "PtAbsMass", "eLOGmOeAbsEta", "eLOGmOeAbsEta", "PtAbsMass"};

  }else if (uncmodel == "FullJMR"){

    // Not including the Topology as it is only applied for QCD-like jets

    UFO_JMR_uncertainties_histo_names = {"JMRUnc_mc20vsmc21_"+kind_of_mc+"_PreRec", "JMRUnc_mc20vsmc23_"+kind_of_mc+"_PreRec", "JMRUnc_mc20FullSimvsmc20AF3_"+kind_of_mc+"_PreRec", "JMRUnc_Noise_PreRec", "JMRUnc_mc20UFOvsmc16UFO_PreRec", "JMR_UncertaintiesOutsideCalibratedRegion_AntiKt10UFOCSSKSoftDropBeta100Zcut10", "JMR_Smoothing_AntiKt10UFOCSSKSoftDropBeta100Zcut10", "JMR_FF_Nom_AntiKt10UFOCSSKSoftDropBeta100Zcut10", "JMR_FF_Stat_AntiKt10UFOCSSKSoftDropBeta100Zcut10", "JMR_FF_Det_AntiKt10UFOCSSKSoftDropBeta100Zcut10", "JMR_FF_MCisr_AntiKt10UFOCSSKSoftDropBeta100Zcut10", "JMR_FF_MCfsr_AntiKt10UFOCSSKSoftDropBeta100Zcut10", "JMR_FF_MCps_AntiKt10UFOCSSKSoftDropBeta100Zcut10", "JMR_FF_MCcr_AntiKt10UFOCSSKSoftDropBeta100Zcut10", "JMR_FF_MCrecoil_AntiKt10UFOCSSKSoftDropBeta100Zcut10", "JMR_FF_MCpthard_AntiKt10UFOCSSKSoftDropBeta100Zcut10"};
    
    uncertainty_names = {"MC20 vs MC21 ("+kind_of_mc+")", "MC20 vs MC23 ("+kind_of_mc+")", "FullSim vs AF3 ("+kind_of_mc+")", "Noise", "MC20 UFO vs MC16 UFO", "Flat smearing 20% (outside)", "FF smoothing", "FF nom.", "FF stat.", "FF Detector", "FF ISR", "FF FSR", "FF PS", "FF Color recon.", "FF recoil", "FF pthard"};
    
    uncertainty_treatment = {"eLOGmOeAbsEta", "eLOGmOeAbsEta", "PtAbsMass", "eLOGmOeAbsEta", "eLOGmOeAbsEta", "PtAbsMass", "PtAbsMass", "PtAbsMass", "PtAbsMass", "PtAbsMass", "PtAbsMass", "PtAbsMass", "PtAbsMass", "PtAbsMass", "PtAbsMass", "PtAbsMass"};

  }

  std::cout << " > Uncertainty model: " << uncmodel << "\n" << std::endl;

  if (uncertainty_names.size() != UFO_JMR_uncertainties_histo_names.size()){
    std::cout << "List of syst histograms and syst names of different sizes.\n"; 
    return;
  }

  std::cout << " > Input uncertainty names and histograms provided \n" << std::endl;

  // A vector with all the histograms for the large-R jet JMR uncertainties (from the tool)
  std::vector<TH3F*> JMR_uncertainties_hists_3d;
  int histocount3d = 0;
  std::vector<TH2D*> JMR_uncertainties_hists_2d;
  int histocount2d = 0;
  int iprime[50] = {};

  for (int i=0; i < std::ssize(uncertainty_names); i++){

    TString histoname = UFO_JMR_uncertainties_histo_names[i].c_str();
    TString unctreatment = uncertainty_treatment[i].c_str();
    if (f_JMR_uncertainties_histograms->Get(histoname) != 0){
      
      if (unctreatment == "eLOGmOeAbsEta"){
        JMR_uncertainties_hists_3d.push_back((TH3F*)f_JMR_uncertainties_histograms->Get(histoname)); 
        iprime[i] = histocount3d;           
        histocount3d += 1;
      }else if (unctreatment == "PtAbsMass"){
        JMR_uncertainties_hists_2d.push_back((TH2D*)f_JMR_uncertainties_histograms->Get(histoname));
        iprime[i] = histocount2d;
        histocount2d += 1;
      }    
      std::cout << "   >> NP read '" << UFO_JMR_uncertainties_histo_names[i] << "' with iprime["<<i<<"] = " <<iprime[i]<< std::endl;
      std::cout << "      >>> Uncertainty treatment = " << unctreatment << std::endl;
      
    }else{
      std::cout << "Can't read input histo." << UFO_JMR_uncertainties_histo_names[i] << "\n"; 
      return;
    }

  }

  std::cout << "\n > Input 2D and 3D histograms are read now \n" << std::endl;

  // Make histogram in 1Dim for the plots
  std::vector<std::unique_ptr<TH1D>> JMR_uncertainties_1Dhists;
  
  for (int i=0; i < std::ssize(uncertainty_names); i++){

    JMR_uncertainties_1Dhists.push_back(std::make_unique<TH1D>(("UFO_"+std::to_string(i)).c_str(),("UFO_"+std::to_string(i)).c_str(), npTbins, &npTbinning[0]));
    auto* hist0 = JMR_uncertainties_1Dhists.at(i).get(); // Cache the pointer

    TString unctreatment = uncertainty_treatment[i].c_str();
    float uncertainty_value = 0;

    if (unctreatment == "eLOGmOeAbsEta"){

      auto* hist1 = JMR_uncertainties_hists_3d.at(iprime[i]); // Cache the pointer   

      for (int bin=1; bin<=npTbins; ++bin){
        double j = hist0->GetBinCenter(bin);
        
        // Calculate the hyperbolic cosine of eta and the energy
        double cosh_eta = std::cosh(eta_bin);
        double energy_bin = sqrt((mass_bin*mass_bin) + (j*j)*pow(cosh_eta,2));

        // If we are out of the range of the input histograms we just take the last value
        double x = energy_bin, y = TMath::Log(mass_bin/energy_bin), z = eta_bin;

        Int_t bin_energy = hist1->GetXaxis()->FindBin(x);
        Int_t bin_mass   = hist1->GetYaxis()->FindBin(y);
        Int_t bin_eta    = hist1->GetZaxis()->FindBin(z);

        // If true, the plot is done as a function of mass instead, and we interprete "mass_bin" as "pT_bin"
        if (pTbin){
          energy_bin = sqrt((j*j) + (mass_bin*mass_bin)*pow(cosh_eta,2));

          // If we are out of the range of the input histograms we just take the last value
          x = energy_bin, y = TMath::Log(j/energy_bin), z = eta_bin;

          bin_energy = hist1->GetXaxis()->FindBin(x);
          bin_mass   = hist1->GetYaxis()->FindBin(y);
          bin_eta    = hist1->GetZaxis()->FindBin(z);          
        }

        // In principle we should not take the fabs(), since there's UP and DOWN, but this is just for illustration
        if (dointerpol){
          uncertainty_value = fabs( Read3DHistogram(hist1, x, y, z) ); // Interpolating as in the tool
        }else{
          uncertainty_value = fabs( hist1->GetBinContent(bin_energy, bin_mass, bin_eta) ); // No interpolation
        }

        // Save the systematic uncertainty
        hist0->SetBinContent(bin, uncertainty_value);

      }

    }else if (unctreatment == "PtAbsMass"){

      auto* hist2 = JMR_uncertainties_hists_2d.at(iprime[i]); // Cache the pointer

      for (int bin=1; bin<=npTbins; ++bin){

        double j = hist0->GetBinCenter(bin);

        // If we are out of the range of the input histograms we just take the last value
        double x = j, y = mass_bin;

        Int_t bin_pt   = hist2->GetXaxis()->FindBin(x);
        Int_t bin_mass = hist2->GetYaxis()->FindBin(y);
        
        // If true, the plot is done as a function of mass instead and the value provided as "mass_bin" is pT
        if (pTbin){
          x = mass_bin, y = j;

          bin_pt   = hist2->GetXaxis()->FindBin(x);
          bin_mass = hist2->GetYaxis()->FindBin(y);
        }
        
        // In principle we should not take the fabs(), since there's UP and DOWN, but this is just for illustration
        if (dointerpol){
          uncertainty_value = fabs( Interpolate2D(hist2, x, y) ); // Interpolating as in the tool    
        }else{
          uncertainty_value = fabs( hist2->GetBinContent(bin_pt, bin_mass) ); // No interpolation
        }
        
        // Save the systematic uncertainty
        hist0->SetBinContent(bin, uncertainty_value);

      }

    }

  }

  std::cout << " > Individual uncertainties evaluated \n" << std::endl;

  ////// The plot with the total uncertainty added in cuadrature
  
  std::unique_ptr<TH1D> JMR_All_uncertainties_1Dhist = std::make_unique<TH1D>("total_uncertainty", "total_uncertainty", npTbins, &npTbinning[0]);

  for (int j=1; j<=JMR_All_uncertainties_1Dhist->GetXaxis()->GetNbins(); j++){

    double total_uncertainty = 0;
    
    for (int i=0; i < std::ssize(uncertainty_names); i++){
      
      double uncval = JMR_uncertainties_1Dhists.at(i)->GetBinContent(j);
      
      total_uncertainty += (uncval) * (uncval);
      //std::cout << " total error bin (comp: i = "<<i<<", j = "<<j<<") = " << uncval << std::endl;
    
    }
      
    total_uncertainty = sqrt(total_uncertainty);
    JMR_All_uncertainties_1Dhist->SetBinContent(j, total_uncertainty);
    //std::cout << " total error bin (j = "<<j<<") = " << total_uncertainty << std::endl;

  }

  // The constant plot with the old uncertainty (0.2 for all pt bins)
    
  std::unique_ptr<TH1D> JMR_old_uncertainties_1Dhist = std::make_unique<TH1D>("Old_uncertainty","Old_uncertainty", npTbins, &npTbinning[0]);

  for (int j=1; j<=JMR_old_uncertainties_1Dhist->GetXaxis()->GetNbins(); j++){
    JMR_old_uncertainties_1Dhist->SetBinContent(j, 0.2);
  }

  std::cout << " > Total uncertainty calculated \n" << std::endl;

  // We will also need a total graph with the sum
    
  gStyle->SetOptStat(0);
  gROOT->Reset();
  gROOT->SetStyle("ATLAS");
  gROOT->ForceStyle();
  gStyle->SetPadLeftMargin(0.15);
  gStyle->SetPadRightMargin(0.08);
  gStyle->SetEndErrorSize(4);
  gStyle->SetPaintTextFormat("4.3f");
  gStyle->SetPalette(8,0, 0.3);

  std::unique_ptr<TCanvas> canvas = std::make_unique<TCanvas>("c1", "c1", 1200, 800);

  // An invisible graph to set the limits of the canvas 
  int nbins = (original_ptmax - original_ptmin)/(1.);
  std::unique_ptr<TH1D> DrawHistogram = std::make_unique<TH1D>("draw_hist","draw_hist_title",nbins, original_ptmin, original_ptmax);

  DrawHistogram->SetMinimum(0);
  DrawHistogram->SetMaximum(1.0);
  DrawHistogram->SetLineWidth(0);//invisible histogram to set the limits of the plot
  if (!pTbin){
    canvas->SetLogx();//Logaritmic scale in the X axis
  }

  DrawHistogram->GetXaxis()->SetTitle("p_{T}^{jet} [GeV]");
  DrawHistogram->GetXaxis()->SetLabelOffset(0.0125);
  DrawHistogram->GetXaxis()->SetNdivisions(510);
  DrawHistogram->GetXaxis()->SetMoreLogLabels(true);
  if (pTbin){
    DrawHistogram->GetXaxis()->SetTitle("m^{jet} [GeV]");
    DrawHistogram->GetXaxis()->SetNdivisions(505);
    DrawHistogram->GetXaxis()->SetMoreLogLabels(false);
  }
  DrawHistogram->GetXaxis()->SetTitleOffset(1.50);

  DrawHistogram->GetYaxis()->SetTitle("Fractional JMR uncertainty");
  DrawHistogram->GetYaxis()->SetLabelOffset(0.0145);
  DrawHistogram->GetYaxis()->SetTitleOffset(1.40);

  // The numbers in the axis are still wrong
  DrawHistogram->Draw();

  //// Plot all the histograms

  JMR_old_uncertainties_1Dhist->SetLineColor(kTeal+5);
  JMR_old_uncertainties_1Dhist->SetLineStyle(1);
  JMR_old_uncertainties_1Dhist->SetLineWidth(2);
  JMR_old_uncertainties_1Dhist->SetFillStyle(4000);
  JMR_old_uncertainties_1Dhist->SetFillColor(kTeal-9);
  JMR_old_uncertainties_1Dhist->Draw("hist same");

  JMR_All_uncertainties_1Dhist->SetLineColor(kBlue+2);
  JMR_All_uncertainties_1Dhist->SetLineStyle(1);
  JMR_All_uncertainties_1Dhist->SetLineWidth(2);
  JMR_All_uncertainties_1Dhist->SetFillStyle(4000);
  JMR_All_uncertainties_1Dhist->SetFillColor(590);
  JMR_All_uncertainties_1Dhist->Draw("hist same");

  for (int index=0; index < std::ssize(uncertainty_names); index++){
    int indexp = index + 2;
    if (index == 8) indexp = 14;
    if (index == 9) indexp = 30;
    if (index == 10) indexp = 43;
    if (index == 11) indexp = 46;

    auto* hist0 = JMR_uncertainties_1Dhists.at(index).get(); // Cache the pointer
    hist0->SetLineColor(indexp);
    hist0->SetLineStyle(index+2);
    hist0->SetLineWidth(2);
    hist0->Draw("hist same");
  }

  //// Plot the text

  // ATLAS label

  TString ATLAS_LABELLING="";
  if (atlas_approved==0){
    ATLAS_LABELLING = "Internal";
  }else if (atlas_approved==1)
    ATLAS_LABELLING = "Preliminary";
  else if (atlas_approved==2){
    ATLAS_LABELLING = "";
  }

  TLatex p;
  TLatex l;
  l.SetNDC();
  l.SetTextFont(72);
  l.SetTextSize(0.05);
  //l.SetTextColor(color);
  p.SetNDC();
  p.SetTextFont(42);
  p.SetTextSize(0.05);
  //ATLAS_LABEL(0.2,0.6,kBlack);

  l.DrawLatex(0.67,0.89,"ATLAS");
  p.DrawLatex(0.67+0.115,0.89,ATLAS_LABELLING);

  // Other information

  TLatex data_text;
  data_text.SetNDC();
  data_text.SetTextFont(42);
  data_text.SetTextSize(0.04);
  data_text.DrawLatex(0.18,0.89,"Data 2015-2017, #sqrt{s} = 13 TeV");

  TLatex jet_info_1_text;
  jet_info_1_text.SetNDC();
  jet_info_1_text.SetTextFont(42);
  jet_info_1_text.SetTextSize(0.04);
  jet_info_1_text.DrawLatex(0.18,0.89-0.06,"Phase-I unc. for UFO R = 1.0 jets");

  TLatex jet_info_2_text;
  jet_info_2_text.SetNDC();
  jet_info_2_text.SetTextFont(42);
  jet_info_2_text.SetTextSize(0.04);

  std::string jet_info_2_string = "";
  if (kind_of_mass == "UFO"){
    
    jet_info_2_string = "|#eta^{jet}| = "+TString(Form("%f",eta_bin))+", m_{UFO}^{jet} = "+TString(Form("%i",mass_bin))+" GeV";

	  if (pTbin){
	    jet_info_2_string = "|#eta^{jet}| = "+TString(Form("%f",eta_bin))+", p_{T}^{jet} = "+TString(Form("%i",mass_bin))+" GeV";
    }

  }
  jet_info_2_text.DrawLatex(0.18,0.89-2*0.06,jet_info_2_string.c_str());

  //// Plot the legend
    
  std::unique_ptr<TLegend> legend0_1 = std::make_unique<TLegend>(0.17,0.89-3.33*0.06,0.45,0.89-2.33*0.06);
  legend0_1->SetTextSize(0.035);
  legend0_1->SetTextFont(42);
  legend0_1->SetBorderSize(0);
  legend0_1->SetFillStyle(0);
  legend0_1->AddEntry(JMR_All_uncertainties_1Dhist.get(), "Tot. uncert.","f");
  legend0_1->Draw("same");

  std::unique_ptr<TLegend> legend0_2 = std::make_unique<TLegend>(0.17,0.89-4.33*0.06,0.45,0.89-3.33*0.06);
  legend0_2->SetTextSize(0.035);
  legend0_2->SetTextFont(42);
  legend0_2->SetBorderSize(0);
  legend0_2->SetFillStyle(0);
  legend0_2->AddEntry(JMR_old_uncertainties_1Dhist.get(), "Tot. uncert. (previous)","f");
  legend0_2->Draw("same");

  // List all the uncertainty components
  std::unique_ptr<TLegend> legend1;
  if (uncmodel == "SimpleJMR"){
    legend1 = std::make_unique<TLegend>(0.55,0.70,0.85,0.88);
  }else if (uncmodel == "FullJMR"){
    legend1 = std::make_unique<TLegend>(0.55,0.55,0.85,0.88);
  }
  legend1->SetTextSize(0.035);
  legend1->SetTextFont(42);
  legend1->SetBorderSize(0);
  legend1->SetFillStyle(0);
  for (int index=0; index < std::ssize(uncertainty_names); index++){
    legend1->AddEntry(JMR_uncertainties_1Dhists.at(index).get(), uncertainty_names[index].c_str(),"l");
  }
  legend1->Draw("same");

  DrawHistogram->Draw("axis same");

  std::cout << " > Plots are done now \n" << std::endl;

  std::string outputdir = "./source/JetUncertainties/util/output/";

  try{
      // Check if the directory exists; if not, create it
      if (!std::filesystem::exists(outputdir)) {
          if (std::filesystem::create_directory(outputdir)){
              std::cout << "Output directory created successfully: " << outputdir << std::endl;
          }else{
              std::cerr << "Failed to create directory: " << outputdir << std::endl;
              return; // Exit with error code
          }
      } else {
          std::cout << "Output directory already exists: " << outputdir << std::endl;
      }
  }catch (const std::filesystem::filesystem_error& e){
      std::cerr << "Filesystem error: " << e.what() << std::endl;
      return; // Exit with error code
  }

  std::string string_write;
  std::string string_write_pdf;
  std::string string_write_png;

	TString mBinStr = "mass"+std::to_string(mass_bin);
	if (pTbin){
	  mBinStr = "pT"+std::to_string(mass_bin);
  }

  TString EtaBinStr = "Eta"+std::to_string(eta_bin);

  if (atlas_approved == 0){
    string_write = outputdir+"FractionalJMRuncertainty_"+kind_of_mc+"_"+kind_of_mass+"_"+uncmodel+"_"+EtaBinStr+"_"+mBinStr+ "GeV_ATLASint";
    string_write_pdf = string_write+".pdf";
    string_write_png = string_write+".png";
  }else if (atlas_approved == 1){
    string_write = outputdir+"FractionalJMRuncertainty_"+kind_of_mc+"_"+kind_of_mass+"_"+uncmodel+"_"+EtaBinStr+"_"+mBinStr+ "GeV_ATLASprel";
    string_write_pdf = string_write+".pdf";
    string_write_png = string_write+".png";
  }else if (atlas_approved == 2){
    string_write = outputdir+"FractionalJMRuncertainty_"+kind_of_mc+"_"+kind_of_mass+"_"+uncmodel+"_"+EtaBinStr+"_"+mBinStr+ "GeV_ATLAS";
    string_write_pdf = string_write+".pdf";
    string_write_png = string_write+".png";
  }

  canvas->Print(string_write_pdf.c_str());   
  //canvas->Print(string_write_png.c_str());   

  return;

}

int main(int argc, char* argv[]){

  // Check if enough arguments are passed
  if (argc < 5){
      std::cerr << "Usage: " << argv[0] << " <input1: unc. model> <input2: kind_of_mc> <input3: dointerpol> <input4: eta_bin> <input5: histograms file>" << std::endl;
      return 1; // Exit with an error code
  }

  std::cout << "\n Inputs instructions:" << std::endl;
  std::cout << "      * <input1: unc. model>      : can be 'SimpleJMR' or 'FullJMR' " << std::endl;
  std::cout << "      * <input2: kind_of_mc>      : can be 'MC20', 'MC21', 'MC23', 'MC20AF3' " << std::endl;
  std::cout << "      * <input3: dointerpol>      : can be 'true' or 'false' " << std::endl;
  std::cout << "      * <input4: eta_bin>         : can be any positive decimal number below 2.0 " << std::endl;
  std::cout << "      * <input5: histograms file> : your '.root' conatining the uncertainties " << std::endl;

  // Example: "JMR_MakeUncertaintyPlots SimpleJMR MC20 true 0.1 ./source/JetUncertainties/share/DCM_240502_new/R10_JMR_AllComponents_Phase1.root"

  /////////////////////////////////////////////////////////////
  // Daniel's settings - JMR Phase-I R22 2024 recommendation //
  /////////////////////////////////////////////////////////////

  //// Range of validity of the MC-JES: pT > 150 GeV
  //// Range of validity of the in situ JES & JER: 150 < pT < 2400 GeV
  //// Range of validity of the MC-JMS: pT > 200 GeV
  //// Range of validity of the in sity JES & JMR: 200 < pT < 3000 GeV

  // Type of label ---> 0="Internal", 1= "Preliminar", 2="ATLASLabel"(only "ATLAS")
  int atlas_approved = 0;   

  // Define uncertainty model and conditions
  std::string kind_of_mass = "UFO";
  std::string uncmodel = argv[1];   // "SimpleJMR" or "FullJMR"
  std::string kind_of_mc = argv[2]; // "MC20", "MC21", "MC23", or "MC20AF3"
  std::string stinterpol = argv[3]; // "true", "false"
  
  bool dointerpol = true;
  if (stinterpol == "true"){
    dointerpol = true;
  }else{
    dointerpol = false;
  }

  // - The |eta| value between [the input has 16 bins from 0 and 3.0]
  double eta_bin = std::stod(argv[4]); //0.1, 0.5, 1.05, 1.55, 1.99

  //// Note: when "pTbin = true", this value of mass is interpreted as pT!
  // - If false (true) the plot is done as a function of jet pT (mass)
  bool pTbin = false;
  // - The mass bin (FF has 2 measurements with 50 < mass < 120 GeV or 120 < mass < 300 GeV)
  int mass_bin = 0; 

  //// Histograms input file, example: "./source/JetUncertainties/share/DCM_240502_new/R10_JMR_AllComponents_Phase1.root"
  // Note: this has to be your custom input root file, user and calibration dependent.
  std::string histofile = argv[5];

  ////////////////////////
  // Plotting functions //
  ////////////////////////

  // Plots as a function of jet pT
  pTbin = false; 
  mass_bin = 40;
  MakeJMRPlot(atlas_approved, histofile, uncmodel, kind_of_mass, kind_of_mc, dointerpol, mass_bin, eta_bin, pTbin);

  // Plots as a function of jet mass
  pTbin = true;
  mass_bin = 200;
  MakeJMRPlot(atlas_approved, histofile, uncmodel, kind_of_mass, kind_of_mc, dointerpol, mass_bin, eta_bin, pTbin);

  return 0;
     
}
