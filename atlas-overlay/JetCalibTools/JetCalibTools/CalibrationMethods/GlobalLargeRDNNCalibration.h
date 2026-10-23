/*
   Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

/* ***********************************************************************************\
 *                                                                                   *
 *      Name: GlobalLargeRDNNCalibration                                             *
 *      Purpose: Perform the DNN JES and JMS step of the large-R jets' calibration   *
 *                                                                                   *
 *  #   Date    Comments                   By                                        *
 * -- -------- -------------------------- ------------------------------------------ *
 *  1 31/01/23  First Version              G. Albouy, P.-A. Delsart                  * 
\*************************************************************************************/


#ifndef JetCalibTools_GlobalLargeRDNNCalibration_H
#define JetCalibTools_GlobalLargeRDNNCalibration_H


// Other packages includes
#include "AthOnnxInterfaces/IOnnxRuntimeSvc.h" //Ort::Session

// Local includes
#include "JetCalibTools/JetCalibrationStep.h"

#include <string>
#include <vector>

class EventInfo;
class TEnv;

class GlobalLargeRDNNCalibration : virtual public ::JetCalibrationStep {
  
public:
  // Constructor/destructor/init
    /**
    @brief      The constructor. Note that this doesn't have all the necessary information, so it will not configure things correctly. 
    */
    GlobalLargeRDNNCalibration();      

    /**
    @brief      The constructor. Note that this doesn't have all the necessary information, so it will not configure things correctly. 
    @param name The name of the tool being created
    */
    GlobalLargeRDNNCalibration(const std::string& name);      

    /**
    @brief               The constructor, which is used by the JetCalibrationTool
    @param name          The name of the tool being created
    @param config        The name of the config file for the calibration
    @param jetAlgo       The name of the jet collection
    @param calibAreaTag  The tag for this calibration
    @param dev           A flag for if the calibration is run in development mode
    */
    GlobalLargeRDNNCalibration(const std::string& name, TEnv * config, const TString& calibArea, bool dev);

    /**
    @brief  The destructor
    */
    virtual ~GlobalLargeRDNNCalibration() ;

    /**
    @brief          Returns the charged fraction of a jet
    @param name The name of the tool being created
    */
    virtual StatusCode initialize() override;
  

    /// VarRetriever is a generic class to access Jet and/or JetEventInfo variables.
    ///  This class is expected to be placeholder in use while a proper configurable system to genericaly access variables is developed
    struct VarRetriever;

    protected:
    // @brief          Calibrates the jet, and decorates it with the calibration using the name "JetGNNCScaleMomentum"
    // @param jet_reco The jet
    // @param jetEventInfo A set of information about the event and jet
    virtual StatusCode calibrate(xAOD::Jet& jet, JetEventInfo&) const override;

    private:

    /**
    @brief Returns a vector of input features for the NN
    @param jet_reco  The jet
    @param jetEventInfo A set of information about the event and jet
    */
    std::vector<float> getJetFeatures( xAOD::Jet& jet_reco, JetEventInfo& jetEventInfo) const;

    std::vector<TString> m_NNInputs;
    std::vector<double> m_eScales;
    std::vector<double> m_NormOffsets;
    std::vector<double> m_NormScales;
    std::string m_modelFileName;

    std::vector<VarRetriever*> m_varretrievers;
  
    std::unique_ptr< Ort::Session > m_session;
    std::vector<int64_t> m_input_node_dims;
    std::vector<const char*> m_input_node_names;
    std::vector<int64_t> m_output_node_dims;
    std::vector<const char*> m_output_node_names;
    
    TEnv * m_config{};
    std::string m_calibArea;
    bool m_devMode{};


}; // Class GlobalLargeRDNNCalibration


#endif
