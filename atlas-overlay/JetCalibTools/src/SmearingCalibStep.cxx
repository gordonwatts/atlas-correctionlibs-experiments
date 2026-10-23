///////////////////////// -*- C++ -*- /////////////////////////////

/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

// SmearingCalibStep.cxx 
// Implementation file for class SmearingCalibStep
/////////////////////////////////////////////////////////////////// 

#include "JetCalibTools/SmearingCalibStep.h"


SmearingCalibStep::SmearingCalibStep(const std::string& name)
  : asg::AsgTool( name ){ }


StatusCode SmearingCalibStep::initialize(){

    ATH_MSG_DEBUG ("Initializing " << name() );

    ATH_MSG_DEBUG("Reading from " << m_jetInScale << " and writing to " << m_jetOutScale);

    // Check the specified smearing type    
    if (m_smearType == "")
    {
        ATH_MSG_FATAL("No jet smearing type was specified.  Aborting.");
        return StatusCode::FAILURE;
    }
    else if (m_smearType=="pt")
    {
        m_smearTypeClass = SmearType::Pt;
    }
    else if (m_smearType=="mass")
    {
        m_smearTypeClass = SmearType::Mass;
    }
    else if (m_smearType=="FourVec")
    {
        m_smearTypeClass = SmearType::FourVec;
    }
    else
    {
        ATH_MSG_FATAL("Unrecognized jet smearing type: " << m_smearType << "--> the available options are 'pt', 'mass' or 'FourVec'");
        return StatusCode::FAILURE;
    }

 
    // Retrieve the histograms
    ATH_CHECK( m_histToolMC.retrieve() );
    ATH_CHECK( m_histToolData.retrieve() );

    return StatusCode::SUCCESS;
}

TRandom3* SmearingCalibStep::getTLSRandomGen(unsigned long seed) const
{
  TRandom3* random = m_rand_tls.get();
  if (!random) {
    random = new TRandom3();
    m_rand_tls.reset(random);
  }
  random->SetSeed(seed);
  return random;
}

StatusCode SmearingCalibStep::getSigmaSmear(xAOD::Jet& jet, const JetHelper::JetContext & jc, double& sigmaSmear) const
{
    /*
        Nominal jet smearing
            If sigma_data > sigma_MC, then we want to smear MC to match data
            If sigma_data < sigma_MC, then we do not want to smear data to match MC
            The second case is instead the source of an uncertainty (see JetUncertainties)

        To make MC agree with data:
            if (sigma_data > sigma_MC) then sigma_smear^2 = sigma_data^2 - sigma_MC^2
            if (sigma_data < sigma_MC) then do nothing
            Smearing using a Gaussian centered at 1 and with a width of sigma_smear

        Note that data is never smeared, as blocked in JetCalibrationTool.cxx 
	--> TODO: JetCalibrationTool.cxx throws an error if you try to run smearing calibration on data - include this in new tool
    */

    double resolutionMC = 0;
    if (getNominalResolutionMC(jet, jc, resolutionMC).isFailure())
        return StatusCode::FAILURE;

    double resolutionData = 0;
    if (getNominalResolutionData(jet, jc, resolutionData).isFailure())
        return StatusCode::FAILURE;
 
    // Nominal smearing only if data resolution is larger than MC resolution
    // This is because we want to smear the MC to match the data
    // if MC is larger than data, don't make the nominal data worse, so smear is 0
    if (resolutionMC < resolutionData)
        sigmaSmear = sqrt(resolutionData*resolutionData - resolutionMC*resolutionMC);
    else
        sigmaSmear = 0;
    
    return StatusCode::SUCCESS;
}

StatusCode SmearingCalibStep::getNominalResolutionData(const xAOD::Jet& jet, const JetHelper::JetContext& jc, double& resolution) const
{
    resolution = m_histToolData->getValue(jet, jc); 
    return StatusCode::SUCCESS;
}

StatusCode SmearingCalibStep::getNominalResolutionMC(const xAOD::Jet& jet, const JetHelper::JetContext& jc, double& resolution) const
{
    resolution = m_histToolMC->getValue(jet, jc); 
    return StatusCode::SUCCESS;
}

StatusCode SmearingCalibStep::calibrate(xAOD::JetContainer& jets) const {
    ATH_MSG_DEBUG("Applying smearing calibration step to jet collection.");

    JetHelper::JetContext jc;

    for(const auto jet: jets){

    const xAOD::JetFourMom_t jetStartP4 = jet->getAttribute<xAOD::JetFourMom_t>(m_jetInScale);
    jet->setJetP4(jetStartP4);

    double sigmaSmear = 0;
    if (getSigmaSmear(*jet, jc, sigmaSmear).isFailure())
        return StatusCode::FAILURE;
    
    // Set the random seed deterministically using jet phi
    unsigned long seed = static_cast<unsigned long>(1.e5*fabs(jet->phi()));
    // SetSeed(0) uses the clock, so avoid this
    if(seed == 0) seed = 45583453; // arbitrary number which the seed couldn't otherwise be
    TRandom3* rng = getTLSRandomGen(seed);

    // Get the Gaussian-distributed random number
    // Force this to be a positive value
    // Negative values should be extraordinarily rare, but they do exist
    double smearingFactor = -1;
    while (smearingFactor < 0)
        smearingFactor = rng->Gaus(1.,sigmaSmear);

    xAOD::JetFourMom_t calibP4 = jetStartP4;

    switch (m_smearTypeClass)
    {
        case SmearType::Pt:
            calibP4 = xAOD::JetFourMom_t(jet->pt()*smearingFactor,jet->eta(),jet->phi(),jet->m());
            break;

        case SmearType::Mass:
            calibP4 = xAOD::JetFourMom_t(jet->pt(),jet->eta(),jet->phi(),smearingFactor*jet->m());
            break;

        case SmearType::FourVec:
            calibP4 = xAOD::JetFourMom_t(jet->pt()*smearingFactor,jet->eta(),jet->phi(),jet->m()*smearingFactor);
            break;

        default:
            // We should never reach this, it was checked during initialization
            ATH_MSG_ERROR("Cannot smear the jet, the smearing type was not set");
            return StatusCode::FAILURE;
    }

    // Set the output scale
    jet->setAttribute<xAOD::JetFourMom_t>(m_jetOutScale,calibP4);
    jet->setJetP4(calibP4);
    }

    return StatusCode::SUCCESS;   
}
