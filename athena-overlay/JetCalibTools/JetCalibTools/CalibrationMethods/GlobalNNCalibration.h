/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/


#ifndef JetCalibTools_GlobalNNCalibration_H
#define JetCalibTools_GlobalNNCalibration_H

#include "TH1.h"
#include "TString.h"
#include "lwtnn/LightweightGraph.hh"

// Local includes
#include "JetCalibTools/JetCalibrationStep.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

class JetEventInfo;
class TEnv;


/**
 * @class GlobalNNCalibration
 * @author Jennifer Roloff
 * @date August 2021
 * @brief Tool to provide the global NN calibration for jets
 */





class GlobalNNCalibration : virtual public ::JetCalibrationStep {

  public:
  // Constructor/destructor/init
  /**
   @brief      The constructor. Note that this doesn't have all the necessary information, so it will not configure things correctly. 
  */
  GlobalNNCalibration();      

  /**
   @brief      The constructor. Note that this doesn't have all the necessary information, so it will not configure things correctly. 
   @param name The name of the tool being created
  */
  GlobalNNCalibration(const std::string& name);      

  /**
   @brief               The constructor, which is used by the JetCalibrationTool
   @param name          The name of the tool being created
   @param config        The name of the config file for the calibration
   @param jetAlgo       The name of the jet collection
   @param calibAreaTag  The tag for this calibration
   @param dev           A flag for if the calibration is run in development mode
  */
  GlobalNNCalibration(const std::string& name, TEnv * config, TString jetAlgo, const TString& calibAreaTag, bool dev);

  /**
   @brief  The destructor
  */
  virtual ~GlobalNNCalibration() = default;

  /**
   @brief          Returns the charged fraction of a jet
   @param name The name of the tool being created
  */
  virtual StatusCode initialize() override;


  protected:
  // @brief          Calibrates the jet, and decorates it with the calibration using the name "JetGNNCScaleMomentum"
  // @param jet_reco The jet
  // @param jetEventInfo A set of information about the event and jet
  virtual StatusCode calibrate(xAOD::Jet& jet, JetEventInfo&) const override;

  private:
    
  /**
   @brief          Returns the charged fraction of a jet
   @param jet_reco The jet
   @param PVindex  The index of the PV in the PrimaryVertices container
  */
  double getJetChargedFraction(const xAOD::Jet& jet_reco, int PVindex) const;

  /**
   @brief          Returns the detector eta of the jet
   @param jet_reco The jet
  */
  double getJetDetEta(const xAOD::Jet& jet_reco) const;

  /**
   @brief          Returns the number of tracks with pT > 1 GeV associated to the jet
   @param jet_reco The jet
   @param PVindex  The index of the PV in the PrimaryVertices container
  */
  int getJetNtrk1000(const xAOD::Jet& jet_reco, int PVindex) const;

  /**
   @brief          Returns the jet width
   @param jet_reco The jet
   @param PVindex  The index of the PV in the PrimaryVertices container
  */
  double getJetWtrk1000(const xAOD::Jet& jet_reco, int PVindex) const;

  /**
   @brief          Returns the jet pT after the MCJES calibration
   @param jet_reco The jet
  */
  double getJESPt(const xAOD::Jet& jet_reco) const;

  /**
   @brief Returns a map of possible inputs to the NN, and their corresponding values for this jet
   @param jet_reco  The jet
   @param jetEventInfo A set of information about the event and jet
  */
  std::map<std::string,double> getJetFeatures(const xAOD::Jet& jet_reco, JetEventInfo& jetEventInfo) const;

  /**
   @brief      Gets the slope of the spline histogram for a given eta bin, for extrapolation of the calibration
   @param ieta The eta bin for the spline histogram
   @param minPt The pT at which the extrapolation starts, and where the slope should be calculated
  */
  double getSplineSlope(const int ieta, const double minPt) const;

  /**
   @brief Reads the spline histograms from the file given in the config, and stores them in m_ptCorrFactors
   @param fileName
   @param etajes_name
  */
  void loadSplineHists(const TString & fileName, const std::string &etajes_name = "etaJes");

  /**
   @brief Returns the correction from spline histogram, which should be applied after the NN correction
   @param etaBin
   @param E
  */
  double getSplineCorr(const int etaBin, double E) const;


  /**
   @brief Returns the eta bin, as determined by a list of bin edges
   @param jet_reco The jet 
   @param etaBins A list of bin edges
  */
  int getEtaBin(const xAOD::Jet& jet_reco, const std::vector<double>& etaBins) const;


  std::vector<std::unique_ptr<lwt::LightweightGraph> > m_lwnns;
  std::vector<std::unique_ptr<TH1> > m_ptCorrFactors;
  std::vector<double> m_nnEtaBins;
  std::vector<double> m_closureEtaBins;
  std::vector<TString> m_NNInputs;

  
  TEnv * m_config{};
  TString m_jetAlgo;
  std::string m_calibAreaTag;
  bool m_dev{};
  bool m_doSplineCorr{true};
  bool m_doLogPtScaling{};
  double m_minNNCorrection{};
  double m_maxNNCorrection{};

  std::vector<double> m_JPtS_MinPt_Slopes;
  std::vector<double> m_JPtS_MinPt_Pt;
  std::vector<double> m_JPtS_MinPt_R;


}; // Class GlobalNNCalibration


#endif


