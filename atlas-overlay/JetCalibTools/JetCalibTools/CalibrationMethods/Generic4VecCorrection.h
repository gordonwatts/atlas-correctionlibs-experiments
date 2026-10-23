/*
  Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration
*/

#ifndef JETCALIBTOOLS_GENERIC4VECCORRECTION_H
#define JETCALIBTOOLS_GENERIC4VECCORRECTION_H


#include "TString.h"
#include "TH1.h"

#include "JetCalibTools/JetCalibrationStep.h"

#include <nlohmann/json.hpp>
#include <map>

class TEnv;
class TH2;

class Generic4VecCorrection
    : virtual public JetCalibrationStep
{

  public:

    // Enums for corrections supported by this tool
    enum JET_CORRTYPE{
        UNKNOWN = 0,
        PTRESIDUAL = 1,
        MC2MC = 2,
        FASTSIM = 3
    };

    // Constructor/destructor/init
    Generic4VecCorrection();
    Generic4VecCorrection(const std::string& name, TEnv* config, const TString & jetAlgo, 
      const TString & calibAreaTag, const TString & forceCalibFile, 
      JET_CORRTYPE correctionType, const TString & mcCampaign="", const TString & simFlavour="", 
      int mcDSID=-1, const TString & generatorsInfo="");
    virtual ~Generic4VecCorrection();
    virtual StatusCode initialize() override;
    virtual StatusCode calibrate(xAOD::Jet& jet, JetEventInfo&) const override;

  private:
    // Extract correction value from chosen 2D histogram
    StatusCode readHisto(float& correctionFactor, TH2* h_correction_2D, float x, float y) const;

    // Dedicated initialize functions for supported corrections
    StatusCode initialize_correctionResponse();
    StatusCode initialize_MC2MC();

    // For MC2MC, parse the calibration showerModel from sample metadata
    StatusCode parse_showerModel(std::string& showerModel, int mcDSID, TString generatorsInfo) const;
    
    StatusCode load_json(nlohmann::json& json_object, const std::string& json_filepath) const;

    // Class variables from constructor
    TEnv* m_config;
    const std::string m_jetAlgo;
    const std::string m_calibAreaTag;
    JET_CORRTYPE m_correctionType;
    const std::string m_simFlavour;

    // Variables for MC2MC Correction
    int m_mcDSID{};
    const std::string m_generatorsInfo;
    const std::string m_mcCampaign;
    const std::string m_forceCalibFile; 

    // Option to skip correction if input file does not conform to requested correction
    bool m_skipCorrection{};

    // Input and output jet scales
    std::string m_inJetScale;
    std::string m_outJetScale;

    // Correction histograms
    std::string m_correctionFilePath;
    std::map<int, TH2*> m_correctionHists;  // If several possible corrections
    TH2* m_only_correction_2D{};            // If only one correction
    TAxis m_etaAxis;                         // For finding center of eta bins to avoid eta interpolation

};

#endif
