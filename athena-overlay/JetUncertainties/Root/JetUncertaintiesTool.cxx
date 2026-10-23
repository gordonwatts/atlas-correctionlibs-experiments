#include "adapter_support.hxx"
/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// General package includes
#include "JetUncertainties/JetUncertaintiesTool.h"
#include "JetUncertainties/Helpers.h"
#include "JetUncertainties/UncertaintyEnum.h"
#include "JetUncertainties/ConfigHelper.h"
#include "JetUncertainties/ResolutionHelper.h"
#include "JetUncertainties/CorrelationMatrix.h"

// UncertaintyHistogram types
#include "JetUncertainties/UncertaintyHistogram.h"
#include "JetUncertainties/ValidityHistogram.h"

// UncertaintyComponent types
#include "JetUncertainties/UncertaintyComponent.h"
#include "JetUncertainties/UncertaintyGroup.h"
#include "JetUncertainties/UncertaintySet.h"
#include "JetUncertainties/PtUncertaintyComponent.h"
#include "JetUncertainties/PtEtaUncertaintyComponent.h"
#include "JetUncertainties/PtLogPtMassForTagSFUncertaintyComponent.h"
#include "JetUncertainties/PtAbsMassUncertaintyComponent.h"
#include "JetUncertainties/PtMassUncertaintyComponent.h"
#include "JetUncertainties/PtAbsMassEtaUncertaintyComponent.h"
#include "JetUncertainties/PtMassEtaUncertaintyComponent.h"
#include "JetUncertainties/ELogMassUncertaintyComponent.h"
#include "JetUncertainties/ELogMassEtaUncertaintyComponent.h"
#include "JetUncertainties/PileupUncertaintyComponent.h"
#include "JetUncertainties/FlavourUncertaintyComponent.h"
#include "JetUncertainties/PerJetFlavourUncertaintyComponent.h"
#include "JetUncertainties/PunchthroughUncertaintyComponent.h"
#include "JetUncertainties/ClosebyUncertaintyComponent.h"
#include "JetUncertainties/CombinedMassUncertaintyComponent.h"
#include "JetUncertainties/LargeRTopologyUncertaintyComponent.h"

// xAOD includes
#include "xAODTracking/VertexContainer.h"

// CP interface includes
#include "PATInterfaces/SystematicVariation.h"
#include "PATInterfaces/SystematicRegistry.h"

// ROOT includes
#include "TString.h"
#include "TFile.h"
#include "TDirectory.h"
#include "TROOT.h"
#include "TEnv.h"
#include "TH2D.h"

// C++ includes
#include <unordered_set>

using namespace jet;

//////////////////////////////////////////////////
//                                              //
//  Constructor/destructor/initialization       //
//                                              //
//////////////////////////////////////////////////

JetUncertaintiesTool::JetUncertaintiesTool(const std::string& name)
    : asg::AsgTool(name)
    , m_isInit(false)
    , m_name(name)
    , m_energyScale(1.e-3)
    , m_release("")
    , m_jetDef("")
    , m_mcType("")
    , m_configFile("")
    , m_calibArea("CalibArea-08")
    , m_path("")
    , m_analysisFile("")
    , m_analysisHistPattern("")
    , m_NJetAccessorName("Njet")
    , m_systFilters()
    , m_defAnaFile("")
    , m_refNPV(-1)
    , m_refMu(-1)
    , m_refNPVHist(nullptr)
    , m_refMuHist(nullptr)
    , m_groups()
    , m_recognizedSystematics()
    , m_recommendedSystematics()
    , m_currentSystSet()
    , m_currentUncSet(nullptr)
    , m_systFilterMap()
    , m_systSetMap()
    , m_fileValidHist(nullptr)
    , m_caloMassWeight(nullptr)
    , m_TAMassWeight(nullptr)
    , m_combMassWeightCaloMassDef(CompMassDef::UNKNOWN)
    , m_combMassWeightTAMassDef(CompMassDef::UNKNOWN)
    , m_combMassParam(CompParametrization::UNKNOWN)
    , m_userSeed(0)
    , m_rand()
    , m_isData(true)
    , m_resHelper(nullptr)
    , m_namePrefix("JET_")
    , m_accTagScaleFactor("temp_SF")
    , m_accEffSF("temp_effSF")
    , m_accSigeffSF("temp_sigeffSF")
    , m_accEfficiency("temp_efficiency")
    , m_accTagResult("temp_accept")
    , m_absEtaGluonFraction(true)
    , m_pseudoDataJERsmearingMode(false)
{
    declareProperty("JetDefinition",m_jetDef);
    declareProperty("MCType",m_mcType);
    declareProperty("ConfigFile",m_configFile);
    declareProperty("CalibArea",m_calibArea);
    declareProperty("Path",m_path);
    declareProperty("AnalysisFile",m_analysisFile);
    declareProperty("AnalysisHistPattern",m_analysisHistPattern);
    declareProperty("NJetAccessorName",m_NJetAccessorName);
    declareProperty("VariablesToShift",m_systFilters);
    declareProperty("IsData",m_isData);
    declareProperty("AbsEtaGluonFraction",m_absEtaGluonFraction);
    declareProperty("PseudoDataJERsmearingMode",m_pseudoDataJERsmearingMode);

    ATH_MSG_DEBUG("Creating JetUncertaintiesTool named "<<m_name);

    // Set dummy default systematic (do nothing)
    // Prevents NULL access if user tries to apply correction without first calling function
    if (JetUncertaintiesTool::applySystematicVariation(CP::SystematicSet()) != StatusCode::SUCCESS)
        ATH_MSG_ERROR(Form("Failed to pre-set applySystematicVariation to no variation"));
}

JetUncertaintiesTool::JetUncertaintiesTool(const JetUncertaintiesTool& toCopy) ATLAS_CTORDTOR_NOT_THREAD_SAFE
    : asg::AsgTool(toCopy.m_name+"_copy")
    , m_isInit(toCopy.m_isInit)
    , m_name(toCopy.m_name+"_copy")
    , m_energyScale(1.e-3)
    , m_release(toCopy.m_release)
    , m_jetDef(toCopy.m_jetDef)
    , m_mcType(toCopy.m_mcType)
    , m_configFile(toCopy.m_configFile)
    , m_calibArea(toCopy.m_calibArea)
    , m_path(toCopy.m_path)
    , m_analysisFile(toCopy.m_analysisFile)
    , m_analysisHistPattern(toCopy.m_analysisHistPattern)
    , m_NJetAccessorName(toCopy.m_NJetAccessorName)
    , m_systFilters(toCopy.m_systFilters)
    , m_defAnaFile(toCopy.m_defAnaFile)
    , m_refNPV(toCopy.m_refNPV)
    , m_refMu(toCopy.m_refMu)
    , m_refNPVHist(toCopy.m_refNPVHist?new UncertaintyHistogram(*toCopy.m_refNPVHist):nullptr)
    , m_refMuHist(toCopy.m_refMuHist?new UncertaintyHistogram(*toCopy.m_refMuHist):nullptr)
    , m_groups()
    , m_recognizedSystematics(toCopy.m_recognizedSystematics)
    , m_recommendedSystematics(toCopy.m_recommendedSystematics)
    , m_currentSystSet(toCopy.m_currentSystSet)
    , m_currentUncSet(nullptr)
    , m_systFilterMap()
    , m_systSetMap()
    , m_fileValidHist(toCopy.m_fileValidHist)
    , m_caloMassWeight(nullptr)
    , m_TAMassWeight(nullptr)
    , m_combMassWeightCaloMassDef(CompMassDef::UNKNOWN)
    , m_combMassWeightTAMassDef(CompMassDef::UNKNOWN)
    , m_combMassParam(CompParametrization::UNKNOWN)
    , m_userSeed(toCopy.m_userSeed)
    , m_rand(toCopy.m_rand)
    , m_isData(toCopy.m_isData)
    , m_resHelper(new ResolutionHelper(*toCopy.m_resHelper))
    , m_namePrefix(toCopy.m_namePrefix)
    , m_accTagScaleFactor(toCopy.m_accTagScaleFactor)
    , m_accEffSF(toCopy.m_accEffSF)
    , m_accSigeffSF(toCopy.m_accSigeffSF)
    , m_accEfficiency(toCopy.m_accEfficiency)
    , m_accTagResult(toCopy.m_accTagResult)
    , m_absEtaGluonFraction(toCopy.m_absEtaGluonFraction)
    , m_pseudoDataJERsmearingMode(toCopy.m_pseudoDataJERsmearingMode)
{
    ATH_MSG_DEBUG("Creating copy of JetUncertaintiesTool named "<<m_name);

    for (size_t iGroup = 0; iGroup < toCopy.m_groups.size(); ++iGroup)
        m_groups.push_back(new UncertaintyGroup(*toCopy.m_groups.at(iGroup)));

    if (JetUncertaintiesTool::applySystematicVariation(m_currentSystSet) != StatusCode::SUCCESS)
        ATH_MSG_ERROR(Form("Failed to re-set applySystematicVariation in new tool copy"));
}

JetUncertaintiesTool::~JetUncertaintiesTool()
{
    ATH_MSG_DEBUG(Form("Deleting JetUncertaintiesTool named %s",m_name.c_str()));

    for (size_t iGroup = 0; iGroup < m_groups.size(); ++iGroup)
        JESUNC_SAFE_DELETE(m_groups.at(iGroup));
    m_groups.clear();

    JESUNC_SAFE_DELETE(m_refNPVHist);
    JESUNC_SAFE_DELETE(m_refMuHist);
    JESUNC_SAFE_DELETE(m_fileValidHist);
    JESUNC_SAFE_DELETE(m_caloMassWeight);
    JESUNC_SAFE_DELETE(m_TAMassWeight);

    m_currentUncSet  = nullptr;

    m_systFilterMap.clear();

    std::unordered_map<CP::SystematicSet,UncertaintySet*>::iterator iter;
    for (iter = m_systSetMap.begin(); iter != m_systSetMap.end(); ++iter)
        JESUNC_SAFE_DELETE(iter->second);
    m_systSetMap.clear();

    JESUNC_SAFE_DELETE(m_resHelper);
}

StatusCode JetUncertaintiesTool::setScaleToMeV()
{
    // Ensure it hasn't been initialized yet
    if (m_isInit)
    {
        ATH_MSG_FATAL("Cannot set the energy scale after initialization of tool: " << m_name);
        return StatusCode::FAILURE;
    }

    m_energyScale = 1.e-3;
    return StatusCode::SUCCESS;
}

StatusCode JetUncertaintiesTool::setScaleToGeV()
{
    // Ensure it hasn't been initialized yet
    if (m_isInit)
    {
        ATH_MSG_FATAL("Cannot set the energy scale after initialization of tool: " << m_name);
        return StatusCode::FAILURE;
    }

    m_energyScale = 1;
    return StatusCode::SUCCESS;
}

StatusCode JetUncertaintiesTool::initialize()
{

 if(m_isInit)return StatusCode::FAILURE;
 if(m_jetDef!="AntiKt4EMPFlow" || m_mcType!="MC23" ||
    m_configFile!="rel22/Winter2025_PreRec/R4_CategoryReduction_FullJER.config" || m_userSeed!=0 || !m_systFilters.empty()){
  ATH_MSG_ERROR("No validated CorrectionLib bundle for these uncertainty properties");return StatusCode::FAILURE;
 }
 try{
  auto root=atlasCorrectionBundle();
  m_correctionBackend=std::make_shared<::JetSystematics>(root+"/uncertainty-tables.json.gz",root+"/uncertainty-manifest.json");
  std::set<std::string> bases;
  for(const auto& name:m_correctionBackend->names()){
   if(name=="NOSYS")continue;
   if(name.find("_PseudoData")!=std::string::npos&&!m_pseudoDataJERsmearingMode)continue;
   bases.insert(name.substr(0,name.find("__")));
  }
  for(const auto& name:bases)ATH_CHECK(addAffectingSystematic(CP::SystematicVariation(name,CP::SystematicVariation::CONTINUOUS),true));
 }catch(const std::exception& e){ATH_MSG_ERROR(e.what());return StatusCode::FAILURE;}
 m_isInit=true;return asg::AsgTool::initialize();

}

//////////////////////////////////////////////////
//                                              //
//  Initialization helper methods               //
//                                              //
//////////////////////////////////////////////////

StatusCode JetUncertaintiesTool::addUncertaintyGroup(const ConfigHelper& helper)
{
    const GroupHelper& group = *helper.getGroupInfo();

    // Ensure the group number is specified and doesn't conflict with existing groups
    if (group.groupNum == 0)
    {
        ATH_MSG_ERROR("Group number was not specified for group: " << group.name.Data());
        return StatusCode::FAILURE;
    }
    for (size_t iGroup = 0; iGroup < m_groups.size(); ++iGroup)
        if (m_groups.at(iGroup)->getGroupNum() == group.groupNum)
        {
            ATH_MSG_ERROR("Group number matches previous group (" << m_groups.at(iGroup)->getName().Data() << "): " << group.name.Data());
            return StatusCode::FAILURE;
        }

    // Build the new group
    UncertaintyGroup* toAdd = new UncertaintyGroup(group);
    if (!toAdd)
    {
        ATH_MSG_ERROR("Failed to build new group: " << group.name.Data());
        return StatusCode::FAILURE;
    }

    m_groups.push_back(toAdd);
    if (!m_groups.back()->getSubgroupNum())
    {
        size_t numGroups = 0;
        for (size_t iGroup = 0; iGroup < m_groups.size(); ++iGroup)
            if (!m_groups.at(iGroup)->getSubgroupNum())
                numGroups++;
        ATH_MSG_INFO(Form("%5zu. %-40s : %s",numGroups //m_groups.size()
                                            ,m_groups.back()->getName().Data()
                                            ,m_groups.back()->getDesc().Data() ));
    }
    return StatusCode::SUCCESS;
}

StatusCode JetUncertaintiesTool::addUncertaintyComponent(const ConfigHelper& helper)
{
    const bool isSimpleGroup = helper.isCompGroup();
    const ComponentHelper& component = *helper.getComponentInfo();
    const GroupHelper&     group     = *helper.getGroupInfo();

    ATH_MSG_DEBUG(Form("Starting to process %s named %s",isSimpleGroup?"simple component group":"standard component",component.name.Data()));

    // Find the group index that this component belongs to
    // Note that if this is a simple group, we first need to build the associated group
    if (isSimpleGroup)
    {
        UncertaintyGroup* simpleGroup = new UncertaintyGroup(group);
        if (!simpleGroup)
        {
            ATH_MSG_ERROR("Failed to build simple group for component: " << component.name.Data());
            return StatusCode::FAILURE;
        }
        const size_t groupIndex = m_groups.size();
        m_groups.push_back(simpleGroup);
        ATH_MSG_DEBUG(Form("Created new group \"%s\" for a simple component at index %zu",simpleGroup->getName().Data(),groupIndex));

        if (!m_groups.back()->getSubgroupNum())
        {
            size_t numGroups = 0;
            for (size_t iGroup = 0; iGroup < m_groups.size(); ++iGroup)
                if (!m_groups.at(iGroup)->getSubgroupNum())
                    numGroups++;
            ATH_MSG_INFO(Form("%5zu. %-40s : %s",numGroups //m_groups.size()
                                                ,m_groups.back()->getName().Data()
                                                ,m_groups.back()->getDesc().Data() ));
        }

        // We now have the simple component group
        // Check if we are in the simple case (one component) or more difficult case (sub components)
        if (component.subComps.empty())
        {
            // Easy case, build the component and add it directly
            UncertaintyComponent* compObject = buildUncertaintyComponent(component);
            if (!compObject)
                return StatusCode::FAILURE;

            if (m_groups.at(groupIndex)->addComponent(compObject).isFailure())
                return StatusCode::FAILURE;
            ATH_MSG_DEBUG(Form("Added single component \"%s\" to simple group \"%s\" (index %zu)",compObject->getName().Data(),m_groups.at(groupIndex)->getName().Data(),groupIndex));
        }
        else
        {
            for (size_t iSubComp = 0; iSubComp < component.subComps.size(); ++iSubComp)
            {
                // Build a new ComponentHelper object for each subcomponent
                ComponentHelper subComp(component);
                subComp.uncNames.clear();
                subComp.subComps.clear();
                subComp.name = component.subComps.at(iSubComp);
                subComp.uncNames.push_back(component.subComps.at(iSubComp));

                UncertaintyComponent* subCompObject = buildUncertaintyComponent(subComp);
                if (!subCompObject)
                    return StatusCode::FAILURE;

                if (m_groups.at(groupIndex)->addComponent(subCompObject).isFailure())
                    return StatusCode::FAILURE;
                ATH_MSG_DEBUG(Form("Added component \"%s\" (%zu of %zu) to simple group \"%s\" (index %zu)",subCompObject->getName().Data(),iSubComp+1,component.subComps.size(),m_groups.at(groupIndex)->getName().Data(),groupIndex));
            }
        }
    }
    else
    {
        size_t groupIndex = 0;
        if (m_groups.empty())
        {
            ATH_MSG_ERROR("No groups exist to add the component to: " << component.name.Data());
            return StatusCode::FAILURE;
        }
        for (size_t iGroup = 0; iGroup < m_groups.size(); ++iGroup)
            if (m_groups.at(iGroup)->getGroupNum() == component.groupNum)
            {
                groupIndex = iGroup;
                break;
            }
        if (groupIndex == 0 && m_groups.at(0)->getGroupNum() != component.groupNum)
        {
            ATH_MSG_ERROR("Failed to find group " << component.groupNum << " for the component: " << component.name.Data());
            return StatusCode::FAILURE;
        }

        // We now have the group index where the component belongs
        // Get the component we want to add (complicated function...)
        UncertaintyComponent* compObject = buildUncertaintyComponent(component);
        if (!compObject)
            return StatusCode::FAILURE;

        if (m_groups.at(groupIndex)->addComponent(compObject).isFailure())
            return StatusCode::FAILURE;
        ATH_MSG_DEBUG(Form("Added component \"%s\" to group \"%s\" (index %zu)",compObject->getName().Data(),m_groups.at(groupIndex)->getName().Data(),groupIndex));
    }

    return StatusCode::SUCCESS;
}

UncertaintyComponent* JetUncertaintiesTool::buildUncertaintyComponent(const ComponentHelper& component) const
{
    // Safety checks for required information
    if (component.name == "")
    {
        ATH_MSG_ERROR("Attempting to create a component with no name");
        return nullptr;
    }
    if (component.parametrization == CompParametrization::UNKNOWN)
    {
        ATH_MSG_ERROR("Attempting to create a component with no parametrization: " << component.name.Data());
        return nullptr;
    }
    if (component.scaleVar == CompScaleVar::UNKNOWN)
    {
        ATH_MSG_ERROR("Attempting to create a component with no variable to scale: " << component.name.Data());
        return nullptr;
    }

    // Special cases first
    if (component.isSpecial)
    {
        // First check pileup components
        if (component.pileupType != PileupComp::UNKNOWN)
        {
            // Ensure that the reference values were specified
            if (m_refNPV < 0 && !m_refNPVHist)
            {
                ATH_MSG_ERROR("Attempted to create pileup component without NPV reference value: " << component.name.Data());
                return nullptr;
            }
            if (m_refMu < 0 && !m_refMuHist)
            {
                ATH_MSG_ERROR("Attempted to create pileup component without mu reference value: " << component.name.Data());
                return nullptr;
            }

            if (component.parametrization == CompParametrization::PtEta || component.parametrization == CompParametrization::PtAbsEta)
            {
                if (m_refNPVHist && m_refMuHist)
                    return new PileupUncertaintyComponent(component,m_refNPVHist,m_refMuHist);
                else if (!m_refNPVHist && !m_refMuHist)
                    return new PileupUncertaintyComponent(component,m_refNPV,m_refMu);
                else if (m_refNPVHist && !m_refMuHist)
                    return new PileupUncertaintyComponent(component,m_refNPVHist,m_refMu);
                else if (!m_refNPVHist && m_refMuHist)
                    return new PileupUncertaintyComponent(component,m_refNPV,m_refMuHist);
            }
            else
            {
                ATH_MSG_ERROR(Form("Unexpected parametrization of %s for component %s",CompParametrization::enumToString(component.parametrization).Data(),component.name.Data()));
                return nullptr;
            }
        }
        // Next check flavour components
        else if (component.flavourType != FlavourComp::UNKNOWN)
        {
            if (m_analysisFile.empty())
            {
                ATH_MSG_ERROR("Attempting to create a flavour uncertainty component without having specified an AnalysisRootFile");
                return nullptr;
            }
            else if (component.parametrization == CompParametrization::PtEta || component.parametrization == CompParametrization::PtAbsEta)
            {

                if (component.flavourType == FlavourComp::PerJetResponse ||
                    component.flavourType == FlavourComp::PerJetResponse_Gluon ||
                    component.flavourType == FlavourComp::PerJetResponse_LQ ||
                    component.flavourType == FlavourComp::PerJetResponse_B ||
                    component.flavourType == FlavourComp::PerJetResponse_C){
                        return new PerJetFlavourUncertaintyComponent(component);
                    }else
                        return new FlavourUncertaintyComponent(component,m_jetDef,m_analysisFile.c_str(),m_defAnaFile.c_str(),m_path.c_str(),m_calibArea.c_str(),m_absEtaGluonFraction,m_analysisHistPattern.c_str(),m_NJetAccessorName);

            }
            else
            {
                ATH_MSG_ERROR(Form("Unexpected parametrization of %s for component %s",CompParametrization::enumToString(component.parametrization).Data(),component.name.Data()));
                return nullptr;
            }
        }
        // Next check punchthrough
        else if (component.name.Contains("PunchThrough",TString::kIgnoreCase))
        {
            if (component.parametrization == CompParametrization::PtEta || component.parametrization == CompParametrization::PtAbsEta)
                return new PunchthroughUncertaintyComponent(component);
            else
            {
                ATH_MSG_ERROR(Form("Unexpected parametrization of %s for component %s",CompParametrization::enumToString(component.parametrization).Data(),component.name.Data()));
                return nullptr;
            }
        }
        // Next check closeby
        else if (component.name.Contains("Closeby",TString::kIgnoreCase))
        {
            if (component.parametrization == CompParametrization::Pt)
                return new ClosebyUncertaintyComponent(component);
            else
            {
                ATH_MSG_ERROR(Form("Unexpected parametrization of %s for component %s",CompParametrization::enumToString(component.parametrization).Data(),component.name.Data()));
                return nullptr;
            }
        }
        // Next check combined mass
        else if (component.combMassType != CombMassComp::UNKNOWN)
        {
            // Ensure we have the weights we need for combined mass uncertainties
            if (!m_caloMassWeight || !m_TAMassWeight)
            {
                ATH_MSG_ERROR("Asking to create a combined mass term without specifying weights: " << component.name.Data());
                return nullptr;
            }

            // Create the component
            ComponentHelper combComp(component);
            combComp.name = component.name;
            combComp.uncNames.clear();
            CombinedMassUncertaintyComponent* cmuc = new CombinedMassUncertaintyComponent(combComp);

            // Set the weights
            if (cmuc->setCaloWeights(m_caloMassWeight).isFailure()) return nullptr;
            if (cmuc->setTAWeights(m_TAMassWeight).isFailure())     return nullptr;
            if (cmuc->setCombWeightMassDefs(m_combMassWeightCaloMassDef,m_combMassWeightTAMassDef).isFailure()) return nullptr;
            if (cmuc->setCombWeightParam(m_combMassParam).isFailure())  return nullptr;
            if (component.combMassType == CombMassComp::Calo || component.combMassType == CombMassComp::Both)
            {
                // Define the calorimeter group if applicable
                GroupHelper caloGroupH(component.name+"_CaloGroup");
                caloGroupH.groupNum = 0;
                caloGroupH.subgroupNum = 0;
                caloGroupH.category = CompCategory::UNKNOWN;
                caloGroupH.correlation = CompCorrelation::Correlated;
                caloGroupH.reducible = false;

                UncertaintyGroup* caloGroup = new UncertaintyGroup(caloGroupH);
                if (!caloGroup)
                {
                    ATH_MSG_ERROR("Failed to build calo-group for combined mass component: " << component.name.Data());
                    return nullptr;
                }

                // Get the calo terms and calo mass definitions
                std::vector<TString> caloComps = jet::utils::vectorize<TString>(component.caloMassTerm,", ");
                std::vector<TString> caloMassDefs = jet::utils::vectorize<TString>(component.caloMassDef,", ");
                if (caloComps.size() != caloMassDefs.size())
                {
                    ATH_MSG_ERROR("Unbalanced number of calo mass terms and calo mass definitions, " << caloComps.size() << " vs " << caloMassDefs.size() << " for combined mass component: " << component.name.Data());
                    return nullptr;
                }

                // Build the component(s) and add them directly
                for (size_t iComp = 0; iComp < caloComps.size(); ++iComp)
                {
                    // Prepare the helper
                    ComponentHelper caloCompH(component);
                    caloCompH.uncNames.clear();
                    caloCompH.isSpecial = false;
                    caloCompH.name = caloComps.at(iComp);
                    caloCompH.uncNames.push_back(std::string(caloCompH.name)+"_"+m_jetDef);
                    caloCompH.massDef = CompMassDef::stringToEnum(caloMassDefs.at(iComp));
                    if (caloCompH.massDef == CompMassDef::UNKNOWN)
                    {
                        ATH_MSG_ERROR("Failed to parse calo mass definition " << iComp << " (" << caloMassDefs.at(iComp).Data() << ") for combined mass component: " << component.name.Data());
                        return nullptr;
                    }

                    // Build the component
                    UncertaintyComponent* caloComp = buildUncertaintyComponent(caloCompH);
                    if (!caloComp)
                        return nullptr;

                    if (caloGroup->addComponent(caloComp).isFailure())
                        return nullptr;
                }

                // Done preparations, now set the calo mass group
                if (cmuc->setCaloTerm(caloGroup).isFailure())
                    return nullptr;
            }
            if (component.combMassType == CombMassComp::TA || component.combMassType == CombMassComp::Both)
            {
                // Define the track-assisted group if applicable
                GroupHelper TAGroupH(component.name+"_TAGroup");
                TAGroupH.groupNum = 0;
                TAGroupH.subgroupNum = 0;
                TAGroupH.category = CompCategory::UNKNOWN;
                TAGroupH.correlation = CompCorrelation::Correlated;
                TAGroupH.reducible = false;

                UncertaintyGroup* TAGroup = new UncertaintyGroup(TAGroupH);
                if (!TAGroup)
                {
                    ATH_MSG_ERROR("Failed to build TA-group for combined mass component: " << component.name.Data());
                    return nullptr;
                }

                // Set the TA terms and TA mass definitions
                std::vector<TString> TAComps = jet::utils::vectorize<TString>(component.TAMassTerm,", ");
                std::vector<TString> TAMassDefs = jet::utils::vectorize<TString>(component.TAMassDef,", ");
                if (TAComps.size() != TAMassDefs.size())
                {
                    ATH_MSG_ERROR("Unbalanced number of TA mass terms and TA mass definitions, " << TAComps.size() << " vs " << TAMassDefs.size() << " for combined mass component: " << component.name.Data());
                    return nullptr;
                }

                // Build the component(s) and add them directly
                for (size_t iComp = 0; iComp < TAComps.size(); ++iComp)
                {
                    // Prepare the helper
                    ComponentHelper TACompH(component);
                    TACompH.uncNames.clear();
                    TACompH.isSpecial = false;
                    TACompH.name = TAComps.at(iComp);
                    TACompH.uncNames.push_back(std::string(TACompH.name)+"_"+m_jetDef);
                    TACompH.massDef = CompMassDef::stringToEnum(TAMassDefs.at(iComp));
                    if (TACompH.massDef == CompMassDef::UNKNOWN)
                    {
                        ATH_MSG_ERROR("Failed to parse TA mass definition " << iComp << " (" << TAMassDefs.at(iComp).Data() << ") for combined mass component: " << component.name.Data());
                        return nullptr;
                    }

                    //ATH_MSG_INFO("Creating TA component \"" << TACompH.name.Data() << "\" for combined mass component: " << component.name.Data());

                    // Build the component
                    UncertaintyComponent* TAComp = buildUncertaintyComponent(TACompH);
                    if (!TAComp)
                        return nullptr;

                    if (TAGroup->addComponent(TAComp).isFailure())
                        return nullptr;
                }

                // Done preparations, now set the TA mass group
                if (cmuc->setTATerm(TAGroup).isFailure())
                    return nullptr;
            }

            // Done, return the component
            return cmuc;
        }
        // Next check large-R topology
        else if (component.name.Contains("Large",TString::kIgnoreCase) && component.name.Contains("Topology",TString::kIgnoreCase))
        {
            if (component.parametrization == CompParametrization::PtEta || component.parametrization == CompParametrization::PtAbsEta)
            {
                if (!component.LargeRJetTruthLabels.empty())
                {
                    return new LargeRTopologyUncertaintyComponent(component);
                }
                else
                {
                    ATH_MSG_ERROR(Form("No LargeRJetTruthLabels specified for Large-R jet topology component %s",component.name.Data()));
                    return nullptr;
                }
            }
            else
            {
                ATH_MSG_ERROR(Form("Unexpected parametrization of %s for component %s",CompParametrization::enumToString(component.parametrization).Data(),component.name.Data()));
                return nullptr;
            }
        }
        else
        {
            ATH_MSG_ERROR("Unexpected special component: " << component.name.Data());
            return nullptr;
        }

    }
    // Standard components
    else
    {
        switch(component.parametrization)
        {
            case CompParametrization::Pt:
                return new PtUncertaintyComponent(component);
            case CompParametrization::PtEta:
            case CompParametrization::PtAbsEta:
                return new PtEtaUncertaintyComponent(component);
            case CompParametrization::PtAbsMass:
                return new PtAbsMassUncertaintyComponent(component);
            case CompParametrization::PtMass:
                return new PtMassUncertaintyComponent(component);
            case CompParametrization::PtLOGPtMassForTagSF:
                return new PtLogPtMassForTagSFUncertaintyComponent(component);
            case CompParametrization::PtMassEta:
            case CompParametrization::PtMassAbsEta:
                return new PtMassEtaUncertaintyComponent(component);
            case CompParametrization::PtAbsMassEta:
            case CompParametrization::PtAbsMassAbsEta:
                return new PtAbsMassEtaUncertaintyComponent(component);
            case CompParametrization::eLOGmOe:
                return new ELogMassUncertaintyComponent(component);
            case CompParametrization::eLOGmOeEta:
            case CompParametrization::eLOGmOeAbsEta:
                return new ELogMassEtaUncertaintyComponent(component);
            default:
                ATH_MSG_ERROR("Encountered unexpected parameter type: " << component.param.Data());
                return nullptr;
        }
    }

    ATH_MSG_ERROR("Failed to find the type of component to build: " << component.name.Data());
    return nullptr;
}



//////////////////////////////////////////////////
//                                              //
//  Methods to implement from ISystematicsTool  //
//                                              //
//////////////////////////////////////////////////

bool JetUncertaintiesTool::isAffectedBySystematic(const CP::SystematicVariation& systematic) const
{
    // Compare using basenames to avoid continious vs fixed value comparisons
    const std::set<std::string> baseNames = m_recognizedSystematics.getBaseNames();
    return baseNames.find(systematic.basename()) != baseNames.end();
    //return m_recognizedSystematics.find(systematic) != m_recognizedSystematics.end();
}

CP::SystematicSet JetUncertaintiesTool::affectingSystematics() const
{
    return m_recognizedSystematics;
}

CP::SystematicSet JetUncertaintiesTool::recommendedSystematics() const
{
    return m_recommendedSystematics;
}

CP::SystematicSet JetUncertaintiesTool::appliedSystematics() const
{
    return m_currentSystSet;
}

bool JetUncertaintiesTool::checkIfRecommendedSystematic(const jet::UncertaintyGroup& systematic) const
{
    // Check for things like AFII non-closure in full sim configurations
    if (systematic.isAlwaysZero())
        return false;

    // Check for filters
    bool passesFilter = m_systFilters.empty();
    const std::set<CompScaleVar::TypeEnum> scaleVars = systematic.getScaleVars();

    for (size_t iFilter = 0; iFilter < m_systFilters.size(); ++iFilter)
    {
        if (scaleVars.count(CompScaleVar::stringToEnum(m_systFilters.at(iFilter))))
        {
            passesFilter = true;
            break;
        }
    }
    if (!passesFilter)
        return false;

    // All checked, this is a recommended systematic
    return true;
}

StatusCode JetUncertaintiesTool::addAffectingSystematic(const CP::SystematicVariation& systematic, bool recommended)
{
    CP::SystematicRegistry& registry = CP::SystematicRegistry::getInstance();
    registry.registerSystematic(systematic);
    m_recognizedSystematics.insert(systematic);
    if (recommended)
    {
        m_recommendedSystematics.insert(systematic);
        if (registry.addSystematicToRecommended(systematic) != StatusCode::SUCCESS)
        {
            ATH_MSG_ERROR("Failed to add systematic to list of recommended systematics: " << systematic.name());
            return StatusCode::FAILURE;
        }
    }
    return StatusCode::SUCCESS;
}

StatusCode JetUncertaintiesTool::applySystematicVariation(const CP::SystematicSet& systConfig)
{

 CP::SystematicSet filtered;
 ATH_CHECK(getFilteredSystematicSet(systConfig,filtered));
 if(filtered.size()>1){ATH_MSG_ERROR("Only individual nuisance variations are validated");return StatusCode::FAILURE;}
 m_currentSystSet.swap(filtered);return StatusCode::SUCCESS;

}

StatusCode JetUncertaintiesTool::getFilteredSystematicSet(const CP::SystematicSet& systConfig, CP::SystematicSet& filteredSet)
{
    // Check if we have already encountered this set
    std::unordered_map<CP::SystematicSet,CP::SystematicSet>::iterator iter = m_systFilterMap.find(systConfig);
    if (iter != m_systFilterMap.end())
        filteredSet = iter->second;
    // Make the filtered set and store it
    else
    {
        if (CP::SystematicSet::filterForAffectingSystematics(systConfig,m_recognizedSystematics,filteredSet) != StatusCode::SUCCESS)
            return StatusCode::FAILURE;
        m_systFilterMap.insert(std::make_pair(systConfig,filteredSet));
    }

    return StatusCode::SUCCESS;
}

StatusCode JetUncertaintiesTool::getUncertaintySet(const CP::SystematicSet& filteredSet, jet::UncertaintySet*& uncSet)
{
    // Check if we have already encountered this set
    std::unordered_map<CP::SystematicSet,UncertaintySet*>::iterator iter = m_systSetMap.find(filteredSet);

    // If we have dealt with this set previously, we're done
    if (iter != m_systSetMap.end())
    {
        uncSet = iter->second;
    }
    // Make the new set and store it
    else
    {
        uncSet = new UncertaintySet(filteredSet.name());
        if (uncSet == nullptr || uncSet->initialize(filteredSet,m_groups).isFailure())
        {
            ATH_MSG_ERROR("Failed to create UncertaintySet for filtered CP::SystematicSet: " << filteredSet.name());
            JESUNC_SAFE_DELETE(uncSet);
            return StatusCode::FAILURE;
        }
        m_systSetMap.insert(std::make_pair(filteredSet,uncSet));
    }

    return StatusCode::SUCCESS;
}


//////////////////////////////////////////////////
//                                              //
//  Information retrieval methods               //
//                                              //
//////////////////////////////////////////////////

float JetUncertaintiesTool::getSqrtS() const
{
    float sqrtS = -1;
    const TString release = getRelease().c_str();
    if (release.BeginsWith("2011_"))
        sqrtS = 7000.*m_energyScale;
    else if (release.BeginsWith("2012_"))
        sqrtS = 8000.*m_energyScale;
    else if (release.BeginsWith("2015_") || release.BeginsWith("2016"))
        sqrtS = 13000.*m_energyScale;
    return sqrtS;
}


float JetUncertaintiesTool::getRefMu() const
{
    if (!m_isInit)
    {
        ATH_MSG_FATAL("Tool must be initialized before calling getRefMu");
        return JESUNC_ERROR_CODE;
    }
    if (m_refMuHist)
    {
        ATH_MSG_FATAL("Tool contains a histogram for refMu, cannot return float");
        return JESUNC_ERROR_CODE;
    }
    return m_refMu;
}

float JetUncertaintiesTool::getRefNPV() const
{
    if (!m_isInit)
    {
        ATH_MSG_FATAL("Tool must be initialized before calling getRefNPV");
        return JESUNC_ERROR_CODE;
    }
    if (m_refNPVHist)
    {
        ATH_MSG_FATAL("Tool contains a histogram for refNPV, cannot return float");
        return JESUNC_ERROR_CODE;
    }
    return m_refNPV;
}

float JetUncertaintiesTool::getRefMu(const xAOD::Jet& jet) const
{
    if (!m_isInit)
    {
        ATH_MSG_FATAL("Tool must be initialized before calling getRefMu");
        return JESUNC_ERROR_CODE;
    }
    return m_refMuHist ? m_refMuHist->getValue(fabs(jet.eta())) : m_refMu;
}

float JetUncertaintiesTool::getRefNPV(const xAOD::Jet& jet) const
{
    if (!m_isInit)
    {
        ATH_MSG_FATAL("Tool must be initialized before calling getRefNPV");
        return JESUNC_ERROR_CODE;
    }
    return m_refNPVHist ? m_refNPVHist->getValue(fabs(jet.eta())) : m_refNPV;
}


size_t JetUncertaintiesTool::getNumComponents() const
{
    if (!m_isInit)
    {
        ATH_MSG_FATAL("Tool must be initialized before calling getNumComponents");
        return 0;
    }

    return m_groups.size();
}

size_t JetUncertaintiesTool::getComponentIndex(const std::string& name) const
{
    return getComponentIndex(TString(name.c_str()));
}

size_t JetUncertaintiesTool::getComponentIndex(const TString& name) const
{
    if (!m_isInit)
    {
        ATH_MSG_FATAL("Tool must be initialized before calling getComponentIndex");
        return m_groups.size();
    }

    for (size_t iComp = 0; iComp < m_groups.size(); ++iComp)
        if (m_groups.at(iComp)->getName().CompareTo(name,TString::kIgnoreCase) == 0)
            return iComp;

    ATH_MSG_ERROR("Failed to find index for requested component: " << name.Data());
    return m_groups.size();
}

std::string JetUncertaintiesTool::getComponentName(const size_t index) const
{
    if (!m_isInit)
    {
        ATH_MSG_FATAL("Tool must be initialized before calling getComponentName");
        return "";
    }

    if (index < m_groups.size())
        return m_groups.at(index)->getName().Data();

    ATH_MSG_ERROR("Index out of bounds for component name: " << index);
    return "";
}

std::string JetUncertaintiesTool::getComponentDesc(const size_t index) const
{
    if (!m_isInit)
    {
        ATH_MSG_FATAL("Tool must be initialized before calling getComponentDesc");
        return "";
    }

    if (index < m_groups.size())
        return m_groups.at(index)->getDesc().Data();

    ATH_MSG_ERROR("Index out of bounds for component desc: " << index);
    return "";
}

std::string JetUncertaintiesTool::getComponentCategory(const size_t index) const
{
    if (!m_isInit)
    {
        ATH_MSG_FATAL("Tool must be initialized before calling getComponentCategory");
        return "";
    }

    if (index < m_groups.size())
        return CompCategory::enumToString(m_groups.at(index)->getCategory()).Data();

    ATH_MSG_ERROR("Index out of bounds for component category: " << index);
    return "";
}

bool JetUncertaintiesTool::getComponentIsReducible(const size_t index) const
{
    if (!m_isInit)
    {
        ATH_MSG_FATAL("Tool must be initialized before calling getComponentIsReducible");
        return false;
    }

    if (index < m_groups.size())
        return m_groups.at(index)->getIsReducible();

    ATH_MSG_ERROR("Index out of bounds for component category:  " << index);
    return false;
}

StatusCode JetUncertaintiesTool::checkIndexInput(const size_t index) const
{
    if (!m_isInit)
    {
        ATH_MSG_FATAL("Tool must be initialized before asking for information pertaining to a given component index");
        return StatusCode::FAILURE;
    }

    if (index >= m_groups.size())
    {
        ATH_MSG_ERROR(Form("Index out of bounds, asking for %zu in a container of size %zu",index,m_groups.size()));
        return StatusCode::FAILURE;
    }


    return StatusCode::SUCCESS;
}

bool checkScalesSingleVar(const std::set<CompScaleVar::TypeEnum>& varSet, const CompScaleVar::TypeEnum var)
{
    return varSet.size() == 1 && *(varSet.begin()) == var;
}

bool JetUncertaintiesTool::getComponentScalesFourVec(const size_t index) const
{
    if (checkIndexInput(index).isFailure()) return false;
    return checkScalesSingleVar(m_groups.at(index)->getScaleVars(),CompScaleVar::FourVec);
}
bool JetUncertaintiesTool::getComponentScalesPt(const size_t index) const
{
    if (checkIndexInput(index).isFailure()) return false;
    return checkScalesSingleVar(m_groups.at(index)->getScaleVars(),CompScaleVar::Pt);
}
bool JetUncertaintiesTool::getComponentScalesMass(const size_t index) const
{
    if (checkIndexInput(index).isFailure()) return false;
    return checkScalesSingleVar(m_groups.at(index)->getScaleVars(),CompScaleVar::Mass);
}
bool JetUncertaintiesTool::getComponentScalesD12(const size_t index) const
{
    if (checkIndexInput(index).isFailure()) return false;
    return checkScalesSingleVar(m_groups.at(index)->getScaleVars(),CompScaleVar::D12);
}
bool JetUncertaintiesTool::getComponentScalesD23(const size_t index) const
{
    if (checkIndexInput(index).isFailure()) return false;
    return checkScalesSingleVar(m_groups.at(index)->getScaleVars(),CompScaleVar::D23);
}
bool JetUncertaintiesTool::getComponentScalesTau21(const size_t index) const
{
    if (checkIndexInput(index).isFailure()) return false;
    return checkScalesSingleVar(m_groups.at(index)->getScaleVars(),CompScaleVar::Tau21);
}
bool JetUncertaintiesTool::getComponentScalesTau32(const size_t index) const
{
    if (checkIndexInput(index).isFailure()) return false;
    return checkScalesSingleVar(m_groups.at(index)->getScaleVars(),CompScaleVar::Tau32);
}
bool JetUncertaintiesTool::getComponentScalesTau21WTA(const size_t index) const
{
    if (checkIndexInput(index).isFailure()) return false;
    return checkScalesSingleVar(m_groups.at(index)->getScaleVars(),CompScaleVar::Tau21WTA);
}
bool JetUncertaintiesTool::getComponentScalesTau32WTA(const size_t index) const
{
    if (checkIndexInput(index).isFailure()) return false;
    return checkScalesSingleVar(m_groups.at(index)->getScaleVars(),CompScaleVar::Tau32WTA);
}
bool JetUncertaintiesTool::getComponentScalesD2Beta1(const size_t index) const
{
    if (checkIndexInput(index).isFailure()) return false;
    return checkScalesSingleVar(m_groups.at(index)->getScaleVars(),CompScaleVar::D2Beta1);
}
bool JetUncertaintiesTool::getComponentScalesC2Beta1(const size_t index) const
{
    if (checkIndexInput(index).isFailure()) return false;
    return checkScalesSingleVar(m_groups.at(index)->getScaleVars(),CompScaleVar::C2Beta1);
}
bool JetUncertaintiesTool::getComponentScalesQw(const size_t index) const
{
    if (checkIndexInput(index).isFailure()) return false;
    return checkScalesSingleVar(m_groups.at(index)->getScaleVars(),CompScaleVar::Qw);
}
bool JetUncertaintiesTool::getComponentScalesTagScaleFactor(const size_t index) const
{
    if (checkIndexInput(index).isFailure()) return false;
    return checkScalesSingleVar(m_groups.at(index)->getScaleVars(),CompScaleVar::TagScaleFactor);
}
bool JetUncertaintiesTool::getComponentScalesMultiple(const size_t index) const
{
    if (checkIndexInput(index).isFailure()) return false;
    return m_groups.at(index)->getScaleVars().size() > 1;
}
std::set<CompScaleVar::TypeEnum> JetUncertaintiesTool::getComponentScaleVars(const size_t index) const
{
    if (checkIndexInput(index).isFailure()) return {};
    return m_groups.at(index)->getScaleVars();
}
JetTopology::TypeEnum JetUncertaintiesTool::getComponentTopology(const size_t index) const
{
    if (checkIndexInput(index).isFailure()) return JetTopology::UNKNOWN;
    return m_groups.at(index)->getTopology();
}


bool JetUncertaintiesTool::getValidity(size_t index, const xAOD::Jet& jet) const
{
    return getValidity(index,jet,CompScaleVar::UNKNOWN);
}
bool JetUncertaintiesTool::getValidity(size_t index, const xAOD::Jet& jet, const xAOD::EventInfo& eInfo) const
{
    return getValidity(index,jet,eInfo,CompScaleVar::UNKNOWN);
}
bool JetUncertaintiesTool::getValidity ATLAS_NOT_THREAD_SAFE (size_t index, const xAOD::Jet& jet, const CompScaleVar::TypeEnum scaleVar) const
{
    const xAOD::EventInfo* eInfo = getDefaultEventInfo();
    if (!eInfo) return false;
    return getValidity(index,jet,*eInfo,scaleVar);
}
bool JetUncertaintiesTool::getValidity(size_t index, const xAOD::Jet& jet, const xAOD::EventInfo& eInfo, const CompScaleVar::TypeEnum scaleVar) const
{
    if (!m_isInit)
    {
        ATH_MSG_FATAL("Tool must be initialized before calling getValidity");
        return false;
    }

    // Ensure we are within bounds
    if (index >= m_groups.size())
    {
        ATH_MSG_ERROR("Index out of bounds for validity: " << index);
        return false;
    }

    // Check for a global validity histogram
    if (m_fileValidHist && !m_fileValidHist->getValidity(jet))
        return false;

    // Deal with different possible scale types
    // If scaleVar is unknown, work if comp is just one type
    // If scaleVar is specified, request that specific type regardless
    if (scaleVar == CompScaleVar::UNKNOWN)
    {
        if (m_groups.at(index)->getScaleVars().size() != 1)
        {
            ATH_MSG_ERROR("Asked for the validity of a set which scales multiple variables without specifying the variable of interest:" << m_groups.at(index)->getName().Data());
            return false;
        }
        return m_groups.at(index)->getValidity(jet,eInfo,*(m_groups.at(index)->getScaleVars().begin()));
    }
    return m_groups.at(index)->getValidity(jet,eInfo,scaleVar);
}



double JetUncertaintiesTool::getUncertainty(size_t index, const xAOD::Jet& jet) const
{
    return getUncertainty(index,jet,CompScaleVar::UNKNOWN);
}
double JetUncertaintiesTool::getUncertainty(size_t index, const xAOD::Jet& jet, const xAOD::EventInfo& eInfo) const
{
    return getUncertainty(index,jet,eInfo,CompScaleVar::UNKNOWN);
}
double JetUncertaintiesTool::getUncertainty ATLAS_NOT_THREAD_SAFE (size_t index, const xAOD::Jet& jet, const CompScaleVar::TypeEnum scaleVar) const
{
    const xAOD::EventInfo* eInfo = getDefaultEventInfo();
    if (!eInfo) return JESUNC_ERROR_CODE;
    return getUncertainty(index,jet,*eInfo,scaleVar);
}
double JetUncertaintiesTool::getUncertainty(size_t index, const xAOD::Jet& jet, const xAOD::EventInfo& eInfo, const CompScaleVar::TypeEnum scaleVar) const
{
    if (!m_isInit)
    {
        ATH_MSG_FATAL("Tool must be initialized before calling getUncertainty");
        return JESUNC_ERROR_CODE;
    }

    // Ensure we are within bounds
    if (index >= m_groups.size())
    {
        ATH_MSG_ERROR("Index out of bounds for uncertainty: " << index);
        return JESUNC_ERROR_CODE;
    }

    // Watch for a global validity histogram
    if (m_fileValidHist && !m_fileValidHist->getValidity(jet))
    {
        ATH_MSG_ERROR("Jet is out of validity bounds for uncertainty: " << index);
        return JESUNC_ERROR_CODE;
    }


    // Deal with different possible scale types
    // If scaleVar is unknown, work if comp is just one type
    // If scaleVar is specified, request that specific type regardless
    if (scaleVar == CompScaleVar::UNKNOWN)
    {
        if (m_groups.at(index)->getScaleVars().size() != 1)
        {
            ATH_MSG_ERROR("Asked for the uncertainty of a set which scales multiple variables without specifying the variable of interest:" << m_groups.at(index)->getName().Data());
            return JESUNC_ERROR_CODE;
        }
        return m_groups.at(index)->getUncertainty(jet,eInfo,*(m_groups.at(index)->getScaleVars().begin()));
    }
    return m_groups.at(index)->getUncertainty(jet,eInfo,scaleVar);
}



bool JetUncertaintiesTool::getValidUncertainty(size_t index, double& unc, const xAOD::Jet& jet) const
{
    return getValidUncertainty(index, unc, jet, CompScaleVar::UNKNOWN);
}
bool JetUncertaintiesTool::getValidUncertainty(size_t index, double& unc, const xAOD::Jet& jet, const xAOD::EventInfo& eInfo) const
{
    return getValidUncertainty(index, unc, jet, eInfo, CompScaleVar::UNKNOWN);
}
bool JetUncertaintiesTool::getValidUncertainty ATLAS_NOT_THREAD_SAFE (size_t index, double& unc, const xAOD::Jet& jet, const CompScaleVar::TypeEnum scaleVar) const
{
    const xAOD::EventInfo* eInfo = getDefaultEventInfo();
    if (!eInfo) return false;
    return getValidUncertainty(index,unc,jet,*eInfo,scaleVar);
}
bool JetUncertaintiesTool::getValidUncertainty(size_t index, double& unc, const xAOD::Jet& jet, const xAOD::EventInfo& eInfo, const CompScaleVar::TypeEnum scaleVar) const
{
    if (!m_isInit)
    {
        ATH_MSG_FATAL("Tool must be initialized before calling getValidUncertainty");
        return false;
    }

    // Ensure we are within bounds
    if (index >= m_groups.size())
    {
        ATH_MSG_ERROR("Index out of bounds for valid uncertainty: " << index);
        return false;
    }

    // Check for a global validity histogram
    if (m_fileValidHist && !m_fileValidHist->getValidity(jet))
        return false;


    // Deal with different possible scale types
    // If scaleVar is unknown, work if comp is just one type
    // If scaleVar is specified, request that specific type regardless
    if (scaleVar == CompScaleVar::UNKNOWN)
    {
        if (m_groups.at(index)->getScaleVars().size() != 1)
        {
            ATH_MSG_ERROR("Asked for the valid uncertainty of a set which scales multiple variables without specifying the variable of interest:" << m_groups.at(index)->getName().Data());
            return JESUNC_ERROR_CODE;
        }
        return m_groups.at(index)->getValidUncertainty(unc,jet,eInfo,*(m_groups.at(index)->getScaleVars().begin()));
    }
    return m_groups.at(index)->getValidUncertainty(unc,jet,eInfo,scaleVar);
}


double JetUncertaintiesTool::getNominalResolutionMC(const xAOD::Jet& jet, const CompScaleVar::TypeEnum smearType, const JetTopology::TypeEnum topology) const
{
    return getNominalResolution(jet,smearType,topology,true);
}

double JetUncertaintiesTool::getNominalResolutionData(const xAOD::Jet& jet, const CompScaleVar::TypeEnum smearType, const JetTopology::TypeEnum topology) const
{
    return getNominalResolution(jet,smearType,topology,false);
}

double JetUncertaintiesTool::getNominalResolution(const xAOD::Jet& jet, const CompScaleVar::TypeEnum smearType, const JetTopology::TypeEnum topology, const bool readMC) const
{
    if (!m_isInit)
    {
        ATH_MSG_FATAL("Tool must be initialized before calling getNominalResolution");
        return JESUNC_ERROR_CODE;
    }
    if (!m_resHelper)
    {
        ATH_MSG_ERROR("The ResolutionHelper class was not created");
        return JESUNC_ERROR_CODE;
    }

    // Get the nominal histogram, parametrization, and mass def (if relevant) from the helper
    std::tuple<const UncertaintyHistogram*,CompParametrization::TypeEnum,CompMassDef::TypeEnum> resolution = m_resHelper->getNominalResolution(smearType,topology,readMC);

    // Check that we retrieved them successfully
    if (!std::get<0>(resolution) || std::get<1>(resolution) == CompParametrization::UNKNOWN)
        return JESUNC_ERROR_CODE;
    if (CompParametrization::includesMass(std::get<1>(resolution)) && std::get<2>(resolution) == CompMassDef::UNKNOWN)
    {
        // We should never reach this, as it was also checked during initialization
        ATH_MSG_ERROR("Parametrization involves mass but mass def is unknown");
        return JESUNC_ERROR_CODE;
    }

    // Now read the uncertainty from the histogram
    return readHistoFromParam(jet,*std::get<0>(resolution),std::get<1>(resolution),std::get<2>(resolution));
}


double JetUncertaintiesTool::readHistoFromParam(const xAOD::Jet& jet, const UncertaintyHistogram& histo, const CompParametrization::TypeEnum param, const jet::CompMassDef::TypeEnum massDef) const
{
    // Simple case (no mass dependence)
    if (!CompParametrization::includesMass(param))
        return readHistoFromParam(jet.jetP4(),histo,param);

    // Complex case (need to check the mass type to use)
    // Simple four-vector case
    if (massDef == CompMassDef::UNKNOWN || massDef == CompMassDef::FourVecMass)
        return readHistoFromParam(jet.jetP4(),histo,param);
    // Special scale case
    JetFourMomAccessor massScaleAccessor(CompMassDef::getJetScaleString(massDef).Data());
    return readHistoFromParam(massScaleAccessor(jet),histo,param);
}

double JetUncertaintiesTool::readHistoFromParam(const xAOD::JetFourMom_t& jet4vec, const UncertaintyHistogram& histo, const CompParametrization::TypeEnum param) const
{
    double value = 0;
    switch (param)
    {
        case CompParametrization::Pt:
            value = histo.getValue(jet4vec.Pt()*m_energyScale);
            break;
        case CompParametrization::PtEta:
            value = histo.getValue(jet4vec.Pt()*m_energyScale,jet4vec.Eta());
            break;
        case CompParametrization::PtAbsEta:
            value = histo.getValue(jet4vec.Pt()*m_energyScale,fabs(jet4vec.Eta()));
            break;
        case CompParametrization::PtAbsMass:
            value = histo.getValue(jet4vec.Pt()*m_energyScale,jet4vec.M()*m_energyScale);
            break;
        case CompParametrization::PtMass:
            value = histo.getValue(jet4vec.Pt()*m_energyScale,jet4vec.M()/jet4vec.Pt());
            break;
        case CompParametrization::PtLOGPtMassForTagSF:
	    value = histo.getValue(jet4vec.Pt()*m_energyScale,log(jet4vec.M()/jet4vec.Pt()));
            break;
        case CompParametrization::PtMassEta:
            value = histo.getValue(jet4vec.Pt()*m_energyScale,jet4vec.M()/jet4vec.Pt(),jet4vec.Eta());
            break;
        case CompParametrization::PtMassAbsEta:
            value = histo.getValue(jet4vec.Pt()*m_energyScale,jet4vec.M()/jet4vec.Pt(),fabs(jet4vec.Eta()));
            break;
        case CompParametrization::PtAbsMassEta:
            value = histo.getValue(jet4vec.Pt()*m_energyScale,jet4vec.M()*m_energyScale,jet4vec.Eta());
            break;
        case CompParametrization::PtAbsMassAbsEta:
            value = histo.getValue(jet4vec.Pt()*m_energyScale,jet4vec.M()*m_energyScale,fabs(jet4vec.Eta()));
            break;
        case CompParametrization::eLOGmOe:
            value = histo.getValue(jet4vec.E()*m_energyScale,log(jet4vec.M()/jet4vec.E()));
            break;
        case CompParametrization::eLOGmOeEta:
            value = histo.getValue(jet4vec.E()*m_energyScale,log(jet4vec.M()/jet4vec.E()),jet4vec.Eta());
            break;
        case CompParametrization::eLOGmOeAbsEta:
            value = histo.getValue(jet4vec.E()*m_energyScale,log(jet4vec.M()/jet4vec.E()),fabs(jet4vec.Eta()));
            break;
        default:
            ATH_MSG_ERROR("Failed to read histogram due to unknown parametrization type in " << getName());
            break;
    }
    return value;
}


double JetUncertaintiesTool::getNormalizedCaloMassWeight(const xAOD::Jet& jet) const
{
    if (!m_caloMassWeight || !m_TAMassWeight) return 0;

    static const JetFourMomAccessor caloScale (CompMassDef::getJetScaleString(m_combMassWeightCaloMassDef).Data());
    static const JetFourMomAccessor TAScale (CompMassDef::getJetScaleString(m_combMassWeightTAMassDef).Data());

    const double caloRes = m_caloMassWeight ? readHistoFromParam(caloScale(jet),*m_caloMassWeight,m_combMassParam) : 0;
    const double TARes   = m_TAMassWeight ? readHistoFromParam(TAScale(jet),*m_TAMassWeight,m_combMassParam) : 0;

    if (caloRes == 0 || TARes == 0) return 0;

    const double caloFactor = (caloRes == 0) ? 0 : 1./(caloRes*caloRes);
    const double TAFactor   = ( TARes  == 0) ? 0 : 1./(TARes*TARes);

    if (caloFactor + TAFactor == 0) return 0;

    return caloFactor/(caloFactor+TAFactor);
}

double JetUncertaintiesTool::getNormalizedTAMassWeight(const xAOD::Jet& jet) const
{
    if (!m_caloMassWeight || !m_TAMassWeight) return 0;

    static const JetFourMomAccessor caloScale (CompMassDef::getJetScaleString(m_combMassWeightCaloMassDef).Data());
    static const JetFourMomAccessor TAScale (CompMassDef::getJetScaleString(m_combMassWeightTAMassDef).Data());


    const double caloRes = m_caloMassWeight->getValue(caloScale.pt(jet)*m_energyScale,caloScale.m(jet)/caloScale.pt(jet) );
    const double TARes   = m_TAMassWeight->getValue(TAScale.pt(jet)*m_energyScale,TAScale.m(jet)/TAScale.pt(jet));

    if (caloRes == 0 || TARes == 0) return 0;

    const double caloFactor = 1./(caloRes*caloRes);
    const double TAFactor   = 1./(TARes*TARes);

    if (caloFactor + TAFactor == 0) return 0;

    return TAFactor/(caloFactor+TAFactor);
}

//////////////////////////////////////////////////
//                                              //
//  Mulit-component retrieval methods           //
//                                              //
//////////////////////////////////////////////////

std::vector<std::string> JetUncertaintiesTool::getComponentCategories() const
{
    if (!m_isInit)
    {
        ATH_MSG_FATAL("Tool must be initialized before calling getComponentCategories");
        return {};
    }

    // Internally use a set for speed
    // Use std::string rather than CompCategory::TypeEnum because std::string has a hash
    // Hashed access should mean there is no speed difference between using the two types
    std::unordered_set<std::string> categories;
    for (size_t iComp = 0; iComp < m_groups.size(); ++iComp)
        categories.insert(CompCategory::enumToString(m_groups.at(iComp)->getCategory()).Data());

    // Convert the set to a vector
    std::vector<std::string> categoryStrings;
    for (std::unordered_set<std::string>::const_iterator iter = categories.begin() ; iter != categories.end(); ++iter)
        categoryStrings.push_back(*iter);

    return categoryStrings;
}

std::vector<size_t> JetUncertaintiesTool::getComponentsInCategory(const std::string& category) const
{
    if (!m_isInit)
    {
        ATH_MSG_FATAL("Tool must be initialized before calling getComponentsInCategory");
        return {};
    }

    // Internally conver to an enum for both checking and speed of comparison
    const CompCategory::TypeEnum categoryEnum = CompCategory::stringToEnum(category.c_str());
    if (categoryEnum == CompCategory::UNKNOWN)
    {
        ATH_MSG_WARNING("Unrecognized category: " << category);
        return {};
    }

    // Now find the components
    std::vector<size_t> components;
    for (size_t iComp = 0; iComp < m_groups.size(); ++iComp)
        if (m_groups.at(iComp)->getCategory() == categoryEnum)
            components.push_back(iComp);

    return components;
}

std::vector<std::string> JetUncertaintiesTool::getComponentNamesInCategory(const std::string& category) const
{
    if (!m_isInit)
    {
        ATH_MSG_FATAL("Tool must be initialized before calling getComponentNamesInCategory");
        return {};
    }

    std::vector<size_t> components = getComponentsInCategory(category);
    std::vector<std::string> names;
    for (size_t iComp = 0; iComp < components.size(); ++iComp)
        names.push_back(getComponentName(components.at(iComp)));

    return names;
}


//////////////////////////////////////////////////
//                                              //
//  Methods to build the correlation matrix     //
//                                              //
//////////////////////////////////////////////////

TH2D* JetUncertaintiesTool::getPtCorrelationMatrix ATLAS_NOT_THREAD_SAFE (const int numBins, const double minPt, const double maxPt, const double valEta)
{
    return getPtCorrelationMatrix(numBins,minPt,maxPt,valEta,valEta);
}

TH2D* JetUncertaintiesTool::getPtCorrelationMatrix ATLAS_NOT_THREAD_SAFE (const int numBins, const double minPt, const double maxPt, const double valEta1, const double valEta2)
{
    if (!m_isInit)
    {
        ATH_MSG_FATAL("Tool must be initialized before calling getCorrelationMatrix");
        return nullptr;
    }

    std::cout << "Creating with max values " << valEta1 << " " << valEta2 << std::endl;
    CorrelationMatrix corrMat(Form("%s_varpt_eta%.2f_eta%.2f",m_name.c_str(),valEta1,valEta2),numBins,minPt*m_energyScale,maxPt*m_energyScale,valEta1,valEta2);
    if (corrMat.initializeForPt(*this).isFailure())
        return nullptr;
    return new TH2D(*corrMat.getMatrix());
}

TH2D* JetUncertaintiesTool::getEtaCorrelationMatrix ATLAS_NOT_THREAD_SAFE (const int numBins, const double minEta, const double maxEta, const double valPt)
{
    return getEtaCorrelationMatrix(numBins,minEta,maxEta,valPt,valPt);
}

TH2D* JetUncertaintiesTool::getEtaCorrelationMatrix ATLAS_NOT_THREAD_SAFE (const int numBins, const double minEta, const double maxEta, const double valPt1, const double valPt2)
{
    if (!m_isInit)
    {
        ATH_MSG_FATAL("Tool must be initialized before calling getCorrelationMatrix");
        return nullptr;
    }

    CorrelationMatrix corrMat(Form("%s_vareta_pt%.1f_pt%.1f",m_name.c_str(),valPt1/1.e3,valPt2/1.e3),numBins,minEta,maxEta,valPt1*m_energyScale,valPt2*m_energyScale);
    if (corrMat.initializeForEta(*this).isFailure())
        return nullptr;
    return new TH2D(*corrMat.getMatrix());
}


//////////////////////////////////////////////////
//                                              //
//  Methods to apply variations or get a copy   //
//                                              //
//////////////////////////////////////////////////

CP::CorrectionCode JetUncertaintiesTool::applyCorrection ATLAS_NOT_THREAD_SAFE (xAOD::Jet& jet) const
{
    const xAOD::EventInfo* eInfo = getDefaultEventInfo();
    if (!eInfo) return CP::CorrectionCode::Error;
    return applyCorrection(jet,*eInfo);
}

CP::CorrectionCode JetUncertaintiesTool::applyCorrection(xAOD::Jet& jet, const xAOD::EventInfo& eInfo) const
{

 if(!m_isInit||!m_correctionBackend)return CP::CorrectionCode::Error;
 try{
  static const SG::AuxElement::ConstAccessor<float> npv("NPV");
  static const SG::AuxElement::ConstAccessor<char> bjet("IsBjet");
  std::string sys=m_currentSystSet.name();if(sys.empty())sys="NOSYS";
  if(m_isData&&sys.find("JER")!=std::string::npos&&sys.find("_PseudoData")==std::string::npos)sys.insert(sys.find("__"),"_PseudoData");
  auto out=m_correctionBackend->evaluate(sys,{jet.pt(),jet.eta(),jet.phi(),jet.m(),eInfo.averageInteractionsPerCrossing(),npv(eInfo),bjet.isAvailable(jet)?double(bjet(jet)):0.});
  jet.setJetP4(xAOD::JetFourMom_t(out[0],out[1],out[2],out[3]));return CP::CorrectionCode::Ok;
 }catch(const std::exception& e){ATH_MSG_ERROR(e.what());return CP::CorrectionCode::Error;}

}

CP::CorrectionCode JetUncertaintiesTool::correctedCopy ATLAS_NOT_THREAD_SAFE (const xAOD::Jet& input, xAOD::Jet*& output) const
{
    const xAOD::EventInfo* eInfo = getDefaultEventInfo();
    if (!eInfo) return CP::CorrectionCode::Error;
    return correctedCopy(input,output,*eInfo);
}

CP::CorrectionCode JetUncertaintiesTool::correctedCopy(const xAOD::Jet& input, xAOD::Jet*& output, const xAOD::EventInfo& eInfo) const
{
    xAOD::Jet* copy = new xAOD::Jet(input);

    // Call the implemented function
    if (applyCorrection(*copy,eInfo) != CP::CorrectionCode::Ok)
    {
        delete copy;
        return CP::CorrectionCode::Error;
    }
    output = copy;
    return CP::CorrectionCode::Ok;
}

CP::CorrectionCode JetUncertaintiesTool::applyContainerCorrection ATLAS_NOT_THREAD_SAFE (xAOD::JetContainer& inputs) const
{
    const xAOD::EventInfo* eInfo = getDefaultEventInfo();
    if (!eInfo) return CP::CorrectionCode::Error;
    return applyContainerCorrection(inputs,*eInfo);
}

CP::CorrectionCode JetUncertaintiesTool::applyContainerCorrection(xAOD::JetContainer& inputs, const xAOD::EventInfo& eInfo) const
{
    CP::CorrectionCode result = CP::CorrectionCode::Ok;

    // Loop over the container
    for (size_t iJet = 0; iJet < inputs.size(); ++iJet)
    {
        result = applyCorrection(*inputs.at(iJet),eInfo);
        if (result == CP::CorrectionCode::Error)
            break;
    }
    return result;
}

CP::CorrectionCode JetUncertaintiesTool::applyContainerCorrection(
    xAOD::JetContainer& inputs, const CP::SystematicSet& syst) const
{
    std::lock_guard<std::mutex> lock(m_reentrantMutex);
    JetUncertaintiesTool* nc_this ATLAS_THREAD_SAFE =
      const_cast<JetUncertaintiesTool*>(this);
    if (nc_this->applySystematicVariation(syst) != StatusCode::SUCCESS)
        return CP::CorrectionCode::Error;
    const xAOD::EventInfo* eInfo = nullptr;
    if (evtStore()->retrieve(eInfo, "EventInfo").isFailure()) {
        ATH_MSG_ERROR("Failed to retrieve EventInfo in applyContainerCorrection");
        return CP::CorrectionCode::Error;
    }
    return applyContainerCorrection(inputs, *eInfo);
}

const xAOD::EventInfo* JetUncertaintiesTool::getDefaultEventInfo ATLAS_NOT_THREAD_SAFE () const
{
    // NPV decorator(s)
    static const SG::AuxElement::ConstAccessor<float> accNPV("NPV");
    static const SG::AuxElement::Decorator<float> decNPV("NPV");

    // Retrieve the EventInfo object
    static const std::string eiName = "EventInfo";
    const xAOD::EventInfo* ei = nullptr;
    if (evtStore()->retrieve(ei, eiName).isFailure())
    {
        ATH_MSG_ERROR("Failed to retrieve default EventInfo object");
        return nullptr;
    }

    // Check if NPV already exists on const EventInfo object, return if so
    if (accNPV.isAvailable(*ei)) {
        return ei;
    }

    // NPV doesn't already exist, so calculate it
    const xAOD::VertexContainer* vertices = nullptr;
    if (evtStore()->retrieve(vertices,"PrimaryVertices").isFailure())
    {
        ATH_MSG_ERROR("Failed to retrieve default NPV value from PrimaryVertices");
        return nullptr;
    }

    unsigned NPV = 0;
    xAOD::VertexContainer::const_iterator itr;
    for (itr = vertices->begin(); itr != vertices->end(); ++itr)
        if ( (*itr)->nTrackParticles() > 1)
            NPV++;

    // Add NPV to the EventInfo object as a decoration.
    decNPV(*ei) = NPV;

    // Return the decorated object.
    return ei;
}




double JetUncertaintiesTool::getSmearingFactor(const xAOD::Jet& jet, const CompScaleVar::TypeEnum smearType, const double variation) const
{
    /*
        Below follows discussion between B Malaescu, C Young, and S Schramm on 14/08/2018

        sigma_smear^2 = (sigma_nominal + |n*x + m*y + ...|)^2 - (sigma_nominal)^2
            sigma_smear: width of the Gaussian to smear with
            sigma_nominal: the nominal resolution in either (pseudo-)data OR mc (see below)
            n: #sigma variation for NP1
            m: #sigma variation for NP2
            x: uncertainty from NP1
            y: uncertainty from NP2
        Note that n,m,x,y all are signed quantities
            n,m are + for upward variations and - for downward variations
            x,y are + or - to differentiate regions of anticorrelations within a given NP

        As the MC has been smeared to the data in JetCalibTools and we have saved the histograms
        before this smearing has been applied we need to take the maximum of the data and MC
        resolutions to find the nominal resolution which to smear against. For cases when the data
        resolution is better than that in simulation and we additionally are using data as
        pseudo-data (extremely rare in analyses) this might result in a slightly more conservative
        uncertainty.
            sigma_nominal = max(sigma_MC,sigma_data)

        In some cases, it is not desireable to smear (pseudo-)data
        Result: two correlation smearing options, "full" and "simple"
            Full = full correlations preserved, smear both (peudo-)data and MC
            Simple = simplified correlations (= loss of correlations), smear only MC

        In "simple" case, we are smearing MC even if we should ideally smear (pseudo-)data
            sign(nx+my+...)  -->  no longer matters, always use sigma_nominal^MC
        Furthermore, take the absolute value as we are forcing an upward variation
            sigma_smear^2 = (sigma_nom^MC + |n*x + m*y + ...|)^2 - (sigma_nom^MC)^2
            In the case of 1 NP, analysis gets same result if n is positive or negative
            Analysis must symmetrize uncertainties themselves to get downward variations

        The above is all for absolute resolution uncertainties
            In the case of relative resolution uncertainties, not much changes
            n*x --> n*sigma_nominal*x
                n: unchanged from before, #sigma variation for NP1
                x: now this is a fractional uncertainty, but it is still the value of NP1
                sigma_nominal: uncertainty measurement varies (sign unaffected by nominal)
            In other words, all of the above is directly extended

        This all relies on the fact that absolute and relative resolutions are not mixed
            This is enforced by the tool enums, which are one or the other
            Asking for both relative and absolute smearing is two separate scale variables
            The tool checks for such cases and explicitly blocks them

        There is one exception to the above: a "data-MC difference" smearing
            If MC is below data, then we smear MC to match data (nominal smearing)
                Occurs if (sigma_nom^MC - sigma_nom^data) < 0, or equivalently (sigma_nom^data - sigma_nom^MC) > 0
            If data is below MC, we don't want to degrade the resolution of data to match MC
                Occurs if (sigma_nom^MC - sigma_nom^data) > 0, or equivalently (sigma_nom^data - sigma_nom^MC) < 0
            We also can't anti-smear MC (at least not with Gaussian smearing)
        Instead, smear by sigma_smear^2 = (sigma_nom + |Nsigma*[sigma_nom^data - sigma_nom^MC]|)^2 - (sigma_nom)^2
            This uses the second form of the above inequalities to stick with convention
            This way, we signify that we are smearing data (uncertainty < 0 if Nsigma > 0)
        This should be smearing of data (sigma_nom = sigma_nom^data)
            However, in the simple scenario, we still only smear MC
                Apply the uncertainty as-is, with sigma_nom = sigma_nom^MC
                This is actually "conservative" (in magnitude, not necessarily in correlations)
                    |Nsigma*(sigma_nom^data - sigma_nom^MC)| is a fixed value independent of choice, call it X
                    sigma_smear^2 = (sigma_nom + X)^2 - (sigma_nom)^2
                    sigma_smear^2 = sigma_nom^2 [ (1 + X/sigma_nom)^2 - 1 ]
                    Taylor expand: sigma_smear^2 ~ sigma_nom^2 ( 1 + 2*X/sigma_nom - 1 )
                    sigma_smear^2 ~ sigma_nom^2 * 2 X / sigma_nom
                    sigma_smear^2 ~ sigma_nom * 2 X
                    Therefore, larger sigma_nom --> larger sigma_smear
                    In this case, we know that sigma_nom^MC > sigma_nom^data (motivation for uncertainty)
                    As such, sigma_nom = sigma_nom^MC is conservative
        This does not need to be handled any different than other uncertainties in the code
            However, the inputs will need to be made carefully to do the correct thing
            In particular, smear data occurs if sign(unc) < 0
            That is why it is written above as (sigma_nom^data - sigma_nom^MC) < 0
            In other words, the histogram must be created to be <= 0
            It should be <0 when data is below MC, and =0 when data is above MC
            This *could* be calculated on-the-fly from the two nominal histograms
                This requires substantial code development, as now this one NP is different from all others
                In the interest of time and code re-use, instead demand an extra histogram input

        In the end, smear by a Gaussian of mean 1 and width sigma_smear

        ----------

        Given this, the arguments to the function are:
            jet: jet to smear, needed for the kinematic dependence of nominal resolutions
            smearType: the resolution type to select the relevant nominal resolutions
            variation: the signed value of n*x + m*y + ... (for both relative and absolute)

        Dedicated class member variables used by this function are:
            m_isData: needed to control whether or not to smear the jet
            m_resHelper: contains lots of global resolution information
                - Whether to smear MC and data, or only MC
                - Nominal resolution histograms for data
                - Nominal resolution histograms for MC
                - Parametrizations of nominal resolution histograms
            m_rand: the random number generator
            m_userSeed: the optional user-specified seed to use for the random generator

        Technical detail on relative uncertainties:
            The input value of "variation" is always n*x + m*y + ...
            This is trivially correct for absolute uncertainties, but not relative
            However, for relative uncertainties, all NPs are with respect to same nominal
            As such, it factors out, and we can take variation*sigma_nominal^data
            Furthermore, as it factors out, the nominal doesn't matter to determine the sign
            This is important as the sign sets whether data or MC is the nominal
    */

    // Check if we need to do anything at all
    if (variation == 0)
        return 1; // No smearing if the variation is 0
    else if (m_isData)
    {
        if (m_resHelper->smearOnlyMC())
            return 1; // No smearing if this is data and we are in the simple scenario
        if (variation > 0)
            return 1; // No smearing if this is data and the sign says to smear MC
    }
    else if (variation < 0 && !m_resHelper->smearOnlyMC())
        return 1; // No smearing if this is MC and the sign says to smear data and this is not the simple scenario

    // Get the relevant nominal resolution
    const double sigmaMC = getNominalResolution(jet,smearType,m_currentUncSet->getTopology(),true);
    const double sigmaData = getNominalResolution(jet,smearType,m_currentUncSet->getTopology(),false);
    const double sigmaNom = std::max(sigmaMC,sigmaData);

    // If this is a relative uncertainty, get the relevant nominal data histogram
    // This is used to scale the input relative variation to get the absolute impact
    const double relativeFactor = CompScaleVar::isRelResolutionType(smearType) ? sigmaNom : 1;

    // We now have the required information, so let's calculate the smearing factor
    // Note that relativeFactor is 1 if this is an absolute uncertainty
    const double sigmaSmear = sqrt(pow(sigmaNom + fabs(variation)*relativeFactor,2) - pow(sigmaNom,2));

    // Throw an error if the userSeed is set to 1 as this is the seed used in JetCalibTools so leads to correlated smearing
    if (m_userSeed == 1){
        ATH_MSG_ERROR("A seed of 1e5 times the jet phi is used in JetCalibTools so using it here leads to correlated smearing");
        return 0;
    }

    // We have the smearing factor, so prepare to smear
    // If the user specified a seed, then use it times the jet's phi times 1*10^5
    // If not, then use the jet's phi times 2*10^5 in MC, 1.23*10^5 in (pseudo-)data
    // Difference in seed between allows for easy use of pseudo-data
    long long int seed = m_userSeed != 0 ? m_userSeed*1.00e+5*fabs(jet.phi()) : (m_isData ? 1.23e+5 : 2.00e+5)*fabs(jet.phi());
    // SetSeed(0) uses the clock, avoid this
    if(seed == 0) seed = m_isData ? 34545654 : 45583453; // arbitrary numbers which the seed couldn't otherwise be
    m_rand.SetSeed(seed);

    // Calculate and return the smearing factor
    // Force this to be a positive value
    // Negative values should be extraordinarily rare, but they do exist
    double smearingFactor = -1;
    while (smearingFactor < 0)
        smearingFactor = m_rand.Gaus(1.,sigmaSmear);
    return smearingFactor;
}








StatusCode JetUncertaintiesTool::updateSplittingScale12(xAOD::Jet& jet, const double shift) const
{
    static const SG::AuxElement::Accessor<float> accD12("Split12");

    const xAOD::Jet& constJet = jet;
    if (accD12.isAvailable(constJet))
    {
        const float value = accD12(constJet);
        accD12(jet) = shift*value;
        return StatusCode::SUCCESS;
    }

    ATH_MSG_ERROR("Split12 moment (D12) is not available on the jet, please make sure to set Split12 before calling the tool");
    return StatusCode::FAILURE;
}

StatusCode JetUncertaintiesTool::updateSplittingScale23(xAOD::Jet& jet, const double shift) const
{
    static const SG::AuxElement::Accessor<float> accD23("Split23");

    const xAOD::Jet& constJet = jet;
    if (accD23.isAvailable(constJet))
    {
        const float value = accD23(constJet);
        accD23(jet) = shift*value;
        return StatusCode::SUCCESS;
    }

    ATH_MSG_ERROR("Split23 moment (D23) is not available on the jet, please make sure to set Split23 before calling the tool");
    return StatusCode::FAILURE;
}

StatusCode JetUncertaintiesTool::updateTau21(xAOD::Jet& jet, const double shift) const
{
    static const SG::AuxElement::Accessor<float> accTau1("Tau1");
    static const SG::AuxElement::Accessor<float> accTau2("Tau2");
    static const SG::AuxElement::Accessor<float> accTau21("Tau21");
    static const bool Tau21wasAvailable = accTau21.isAvailable(jet);
    static const bool TauNNwasAvailable = accTau2.isAvailable(jet) && accTau1.isAvailable(jet);

    const xAOD::Jet& constJet = jet;
    if (Tau21wasAvailable)
    {
        if (!accTau21.isAvailable(jet))
        {
            ATH_MSG_ERROR("The Tau21 moment was previously available but is not available on this jet.  This functionality is not supported.");
            return StatusCode::FAILURE;
        }
        const float value = accTau21(constJet);
        accTau21(jet) = shift*value;
        return StatusCode::SUCCESS;
    }
    if (TauNNwasAvailable)
    {
        if (! (accTau2.isAvailable(jet) && accTau1.isAvailable(jet)) )
        {
            ATH_MSG_ERROR("The Tau2 and Tau1 moments were previously available but are not available on this jet.  This functionality is not supported.");
            return StatusCode::FAILURE;
        }
        const float tau2 = accTau2(constJet);
        const float tau1 = accTau1(constJet);
        accTau21(jet) = fabs(tau1) > 1.e-6 ? shift*(tau2/tau1) : -999; // 999 to match JetSubStructureMomentTools/NSubjettinessRatiosTool
        return StatusCode::SUCCESS;
    }
    //if (accTau21.isAvailable(constJet))
    //{
    //    const float value = accTau21(constJet);
    //    accTau21(jet) = shift*value;
    //    return StatusCode::SUCCESS;
    //}
    //if (accTau1.isAvailable(constJet) && accTau2.isAvailable(constJet))
    //{
    //    const float value = accTau2(constJet)/accTau1(constJet);
    //    accTau21(jet) = shift*value;
    //    return StatusCode::SUCCESS;
    //}

    ATH_MSG_ERROR("Neither Tau21 nor Tau1+Tau2 moments are available on the jet, please make sure one of these options is available before calling the tool.");
    return StatusCode::FAILURE;
}

StatusCode JetUncertaintiesTool::updateTau32(xAOD::Jet& jet, const double shift) const
{
    static const SG::AuxElement::Accessor<float> accTau2("Tau2");
    static const SG::AuxElement::Accessor<float> accTau3("Tau3");
    static const SG::AuxElement::Accessor<float> accTau32("Tau32");
    static const bool Tau32wasAvailable = accTau32.isAvailable(jet);
    static const bool TauNNwasAvailable = accTau3.isAvailable(jet) && accTau2.isAvailable(jet);

    const xAOD::Jet& constJet = jet;
    if (Tau32wasAvailable)
    {
        if (!accTau32.isAvailable(jet))
        {
            ATH_MSG_ERROR("The Tau32 moment was previously available but is not available on this jet.  This functionality is not supported.");
            return StatusCode::FAILURE;
        }
        const float value = accTau32(constJet);
        accTau32(jet) = shift*value;
        return StatusCode::SUCCESS;
    }
    if (TauNNwasAvailable)
    {
        if (! (accTau3.isAvailable(jet) && accTau2.isAvailable(jet)) )
        {
            ATH_MSG_ERROR("The Tau3 and Tau2 moments were previously available but are not available on this jet.  This functionality is not supported.");
            return StatusCode::FAILURE;
        }
        const float tau3 = accTau3(constJet);
        const float tau2 = accTau2(constJet);
        accTau32(jet) = fabs(tau2) > 1.e-6 ? shift*(tau3/tau2) : -999; // 999 to match JetSubStructureMomentTools/NSubjettinessRatiosTool
        return StatusCode::SUCCESS;
    }
    //if (accTau32.isAvailable(constJet))
    //{
    //    const float value = accTau32(constJet);
    //    accTau32(jet) = shift*value;
    //    return StatusCode::SUCCESS;
    //}
    //if (accTau2.isAvailable(constJet) && accTau3.isAvailable(constJet))
    //{
    //    const float value = accTau3(constJet)/accTau2(constJet);
    //    accTau32(jet) = shift*value;
    //    return StatusCode::SUCCESS;
    //}

    ATH_MSG_ERROR("Neither Tau32 nor Tau2+Tau3 moments are available on the jet, please make sure one of these options is available before calling the tool");
    return StatusCode::FAILURE;
}

StatusCode JetUncertaintiesTool::updateTau21WTA(xAOD::Jet& jet, const double shift) const
{
    static const SG::AuxElement::Accessor<float> accTau1wta("Tau1_wta");
    static const SG::AuxElement::Accessor<float> accTau2wta("Tau2_wta");
    static const SG::AuxElement::Accessor<float> accTau21wta("Tau21_wta");
    static const SG::AuxElement::Accessor<float> accTau1WTA("Tau1_WTA");
    static const SG::AuxElement::Accessor<float> accTau2WTA("Tau2_WTA");
    static const SG::AuxElement::Accessor<float> accTau21WTA("Tau21_WTA");
    static const bool Tau21wtawasAvailable = accTau21wta.isAvailable(jet);
    static const bool Tau21WTAwasAvailable = accTau21WTA.isAvailable(jet);
    static const bool TauNNwtawasAvailable = accTau2wta.isAvailable(jet) && accTau1wta.isAvailable(jet);
    static const bool TauNNWTAwasAvailable = accTau2WTA.isAvailable(jet) && accTau1WTA.isAvailable(jet);

    const xAOD::Jet& constJet = jet;
    if (Tau21wtawasAvailable)
    {
        if (!accTau21wta.isAvailable(jet))
        {
            ATH_MSG_ERROR("The Tau21_wta moment was previously available but is not available on this jet.  This functionality is not supported.");
            return StatusCode::FAILURE;
        }
        const float value = accTau21wta(constJet);
        accTau21wta(jet) = shift*value;
        return StatusCode::SUCCESS;
    }
    if (Tau21WTAwasAvailable)
    {
        if (!accTau21WTA.isAvailable(jet))
        {
            ATH_MSG_ERROR("The Tau21_WTA moment was previously available but is not available on this jet.  This functionality is not supported.");
            return StatusCode::FAILURE;
        }
        const float value = accTau21WTA(constJet);
        accTau21WTA(jet) = shift*value;
        return StatusCode::SUCCESS;
    }
    if (TauNNwtawasAvailable)
    {
        if (! (accTau2wta.isAvailable(jet) && accTau1wta.isAvailable(jet)) )
        {
            ATH_MSG_ERROR("The Tau2_wta and Tau1_wta moments were previously available but are not available on this jet.  This functionality is not supported.");
            return StatusCode::FAILURE;
        }
        const float tau2 = accTau2wta(constJet);
        const float tau1 = accTau1wta(constJet);
        accTau21wta(jet) = fabs(tau1) > 1.e-6 ? shift*(tau2/tau1) : -999; // 999 to match JetSubStructureMomentTools/NSubjettinessRatiosTool
        return StatusCode::SUCCESS;
    }
    if (TauNNWTAwasAvailable)
    {
        if (! (accTau2WTA.isAvailable(jet) && accTau1WTA.isAvailable(jet)) )
        {
            ATH_MSG_ERROR("The Tau2_WTA and Tau1_WTA moments were previously available but are not available on this jet.  This functionality is not supported.");
            return StatusCode::FAILURE;
        }
        const float tau2 = accTau2WTA(constJet);
        const float tau1 = accTau1WTA(constJet);
        accTau21WTA(jet) = fabs(tau1) > 1.e-6 ? shift*(tau2/tau1) : -999; // 999 to match JetSubStructureMomentTools/NSubjettinessRatiosTool
        return StatusCode::SUCCESS;
    }

    ATH_MSG_ERROR("Neither Tau21_wta nor Tau1_wta+Tau2_wta moments are available on the jet, please make sure one of these options is available before calling the tool");
    return StatusCode::FAILURE;
}
StatusCode JetUncertaintiesTool::updateTau32WTA(xAOD::Jet& jet, const double shift) const
{
    static const SG::AuxElement::Accessor<float> accTau2wta("Tau2_wta");
    static const SG::AuxElement::Accessor<float> accTau3wta("Tau3_wta");
    static const SG::AuxElement::Accessor<float> accTau32wta("Tau32_wta");
    static const SG::AuxElement::Accessor<float> accTau2WTA("Tau2_WTA");
    static const SG::AuxElement::Accessor<float> accTau3WTA("Tau3_WTA");
    static const SG::AuxElement::Accessor<float> accTau32WTA("Tau32_WTA");
    static const bool Tau32wtawasAvailable = accTau32wta.isAvailable(jet);
    static const bool Tau32WTAwasAvailable = accTau32WTA.isAvailable(jet);
    static const bool TauNNwtawasAvailable = accTau3wta.isAvailable(jet) && accTau2wta.isAvailable(jet);
    static const bool TauNNWTAwasAvailable = accTau3WTA.isAvailable(jet) && accTau2WTA.isAvailable(jet);

    const xAOD::Jet& constJet = jet;
    if (Tau32wtawasAvailable)
    {
        if (!accTau32wta.isAvailable(jet))
        {
            ATH_MSG_ERROR("The Tau32_wta moment was previously available but is not available on this jet.  This functionality is not supported.");
            return StatusCode::FAILURE;
        }
        const float value = accTau32wta(constJet);
        accTau32wta(jet) = shift*value;
        return StatusCode::SUCCESS;
    }
    if (Tau32WTAwasAvailable)
    {
        if (!accTau32WTA.isAvailable(jet))
        {
            ATH_MSG_ERROR("The Tau32_WTA moment was previously available but is not available on this jet.  This functionality is not supported.");
            return StatusCode::FAILURE;
        }
        const float value = accTau32WTA(constJet);
        accTau32WTA(jet) = shift*value;
        return StatusCode::SUCCESS;
    }
    if (TauNNwtawasAvailable)
    {
        if (! (accTau3wta.isAvailable(jet) && accTau2wta.isAvailable(jet)) )
        {
            ATH_MSG_ERROR("The Tau3_wta and Tau2_wta moments were previously available but are not available on this jet.  This functionality is not supported.");
            return StatusCode::FAILURE;
        }
        const float tau3 = accTau3wta(constJet);
        const float tau2 = accTau2wta(constJet);
        accTau32wta(jet) = fabs(tau2) > 1.e-6 ? shift*(tau3/tau2) : -999; // 999 to match JetSubStructureMomentTools/NSubjettinessRatiosTool
        return StatusCode::SUCCESS;
    }
    if (TauNNWTAwasAvailable)
    {
        if (! (accTau3WTA.isAvailable(jet) && accTau2WTA.isAvailable(jet)) )
        {
            ATH_MSG_ERROR("The Tau3_WTA and Tau2_WTA moments were previously available but are not available on this jet.  This functionality is not supported.");
            return StatusCode::FAILURE;
        }
        const float tau3 = accTau3WTA(constJet);
        const float tau2 = accTau2WTA(constJet);
        accTau32WTA(jet) = fabs(tau2) > 1.e-6 ? shift*(tau3/tau2) : -999; // 999 to match JetSubStructureMomentTools/NSubjettinessRatiosTool
        return StatusCode::SUCCESS;
    }
    //if (accTau32wta.isAvailable(constJet))
    //{
    //    const float value = accTau32wta(constJet);
    //    accTau32wta(jet) = shift*value;
    //    return StatusCode::SUCCESS;
    //}
    //if (accTau32WTA.isAvailable(constJet))
    //{
    //    const float value = accTau32WTA(constJet);
    //    accTau32WTA(jet) = shift*value;
    //    return StatusCode::SUCCESS;
    //}
    //if (accTau2wta.isAvailable(constJet) && accTau3wta.isAvailable(constJet))
    //{
    //    const float value = accTau3wta(constJet)/accTau2wta(constJet);
    //    accTau32wta(jet) = shift*value;
    //    return StatusCode::SUCCESS;
    //}
    //if (accTau2WTA.isAvailable(constJet) && accTau3WTA.isAvailable(constJet))
    //{
    //    const float value = accTau3WTA(constJet)/accTau2WTA(constJet);
    //    accTau32WTA(jet) = shift*value;
    //    return StatusCode::SUCCESS;
    //}

    ATH_MSG_ERROR("Neither Tau32_wta nor Tau2_wta+Tau3_wta moments are available on the jet, please make sure one of these options is available before calling the tool");
    return StatusCode::FAILURE;
}

StatusCode JetUncertaintiesTool::updateD2Beta1(xAOD::Jet& jet, const double shift) const
{
    static const SG::AuxElement::Accessor<float> accD2("D2");
    static const SG::AuxElement::Accessor<float> accECF1("ECF1");
    static const SG::AuxElement::Accessor<float> accECF2("ECF2");
    static const SG::AuxElement::Accessor<float> accECF3("ECF3");
    static const bool D2wasAvailable  = accD2.isAvailable(jet);
    static const bool ECFwasAvailable = accECF1.isAvailable(jet) && accECF2.isAvailable(jet) && accECF3.isAvailable(jet);

    const xAOD::Jet& constJet = jet;
    if (D2wasAvailable)
    {
        if (!accD2.isAvailable(jet))
        {
            ATH_MSG_ERROR("The D2 moment was previously available but is not available on this jet.  This functionality is not supported.");
            return StatusCode::FAILURE;
        }
        const float value = accD2(constJet);
        accD2(jet) = shift*value;
        return StatusCode::SUCCESS;
    }
    if (ECFwasAvailable)
    {
        if (! (accECF1.isAvailable(constJet) && accECF2.isAvailable(constJet) && accECF3.isAvailable(constJet)) )
        {
            ATH_MSG_ERROR("The ECF1, ECF2, and ECF3 moments were previously available but are not available on this jet.  This functionality is not supported.");
            return StatusCode::FAILURE;
        }
        const float ecf1 = accECF1(constJet);
        const float ecf2 = accECF2(constJet);
        const float ecf3 = accECF3(constJet);
        accD2(jet) = fabs(ecf2) > 1.e-6 ? shift * (pow(ecf1/ecf2,3)*ecf3) : -999; // 999 to match JetSubStructureMomentTools/EnergyCorrelatorRatiosTool
        return StatusCode::SUCCESS;
    }

    //if (accD2.isAvailable(constJet))
    //{
    //    const float value = accD2(constJet);
    //    accD2(jet) = shift*value;
    //    return StatusCode::SUCCESS;
    //}
    //if (accECF1.isAvailable(constJet) && accECF2.isAvailable(constJet) && accECF3.isAvailable(constJet))
    //{
    //    const float ecf1 = accECF1(constJet);
    //    const float ecf2 = accECF2(constJet);
    //    const float ecf3 = accECF3(constJet);
    //    accD2(jet) = shift * (pow(ecf1/ecf2,3)*ecf3);
    //    return StatusCode::SUCCESS;
    //}

    ATH_MSG_ERROR("Neither D2 nor ECF1+ECF2+ECF3 moments are available on the jet, please make sure one of these options is available before calling the tool");
    return StatusCode::FAILURE;
}

StatusCode JetUncertaintiesTool::updateC2Beta1(xAOD::Jet& jet, const double shift) const
{
    static const SG::AuxElement::Accessor<float> accC2("C2");
    static const SG::AuxElement::Accessor<float> accECF1("ECF1");
    static const SG::AuxElement::Accessor<float> accECF2("ECF2");
    static const SG::AuxElement::Accessor<float> accECF3("ECF3");
    static const bool C2wasAvailable  = accC2.isAvailable(jet);
    static const bool ECFwasAvailable = accECF1.isAvailable(jet) && accECF2.isAvailable(jet) && accECF3.isAvailable(jet);

    const xAOD::Jet& constJet = jet;
    if (C2wasAvailable)
    {
        if (!accC2.isAvailable(jet))
        {
            ATH_MSG_ERROR("The C2 moment was previously available but is not available on this jet.  This functionality is not supported.");
            return StatusCode::FAILURE;
        }
        const float value = accC2(constJet);
        accC2(jet) = shift*value;
        return StatusCode::SUCCESS;
    }
    if (ECFwasAvailable)
    {
        if (! (accECF1.isAvailable(constJet) && accECF2.isAvailable(constJet) && accECF3.isAvailable(constJet)) )
        {
            ATH_MSG_ERROR("The ECF1, ECF2, and ECF3 moments were previously available but are not available on this jet.  This functionality is not supported.");
            return StatusCode::FAILURE;
        }
        const float ecf1 = accECF1(constJet);
        const float ecf2 = accECF2(constJet);
        const float ecf3 = accECF3(constJet);
        accC2(jet) = fabs(ecf2) > 1.e-6 ? shift * (ecf3*ecf1/pow(ecf2,2)) : -999; // 999 to match JetSubStructureMomentTools/EnergyCorrelatorRatiosTool
        return StatusCode::SUCCESS;
    }

    ATH_MSG_ERROR("Neither C2 nor ECF1+ECF2+ECF3 moments are available on the jet, please make sure one of these options is available before calling the tool");
    return StatusCode::FAILURE;
}

StatusCode JetUncertaintiesTool::updateQw(xAOD::Jet& jet, const double shift) const
{
    static const SG::AuxElement::Accessor<float> accQw("Qw");

    const xAOD::Jet& constJet = jet;
    if (accQw.isAvailable(constJet))
    {
        const float value = accQw(constJet);
        accQw(jet) = shift*value;
        return StatusCode::SUCCESS;
    }

    ATH_MSG_ERROR("Qw moment is not available on the jet, please make sure to set Qw before calling the tool");
    return StatusCode::FAILURE;
}

StatusCode JetUncertaintiesTool::updateTagScaleFactor(xAOD::Jet& jet, const double shift) const
{
    const bool TagScaleFactorwasAvailable  = m_accTagScaleFactor.isAvailable(jet);
    const xAOD::Jet& constJet = jet;
    if (TagScaleFactorwasAvailable)
    {
        if (!m_accTagScaleFactor.isAvailable(jet))
        {
            ATH_MSG_ERROR("TagScaleFactor was previously available but is not available on this jet.  This functionality is not supported.");
            return StatusCode::FAILURE;
        }
        const float value = m_accTagScaleFactor(constJet);
	if ( value < 1e-5 ) {
	  // if the central SF is 0, we don't consider any uncertainties
	  return StatusCode::SUCCESS;
	}
	if (m_accEffSF.isAvailable(jet)) {
	  // if efficiency and efficiency SF are available, inefficiency SF will be calculated
	  const float effSF = m_accEffSF(constJet);
	  const float efficiency = m_accEfficiency(constJet);


	  const bool tagResult = m_accTagResult(constJet);
	  if ( tagResult ){
	    // update the efficiency SF

	    if ( shift*value < 0.0 ){
	      m_accTagScaleFactor(jet) = 0.0;
	    } else {
	      m_accTagScaleFactor(jet) = shift*value;
	    }
	    return StatusCode::SUCCESS;
	  } else {
	    // this jet is "failed", since SF applied to the event is (1-effSF*efficiency)/(1-efficiency)
	    // so inefficiency SF will be recalculated for given uncertainty
	    if ( efficiency < 1.0 ){
	      if ( shift*value < 0.0 ){
		m_accTagScaleFactor(jet) = 1.0/(1. - efficiency);
	      } else {
		m_accTagScaleFactor(jet) = (1. - shift*effSF*efficiency) / (1. - efficiency);
	      }
	    }
	    return StatusCode::SUCCESS;
	  }
	} else {
	  // if efficiency and efficiency SF are NOT available, inefficiency SF will not be calculated

	  if ( shift*value < 0.0 ){
	    m_accTagScaleFactor(jet) = 0.0;
	  } else {
	    m_accTagScaleFactor(jet) = shift*value;
	  }
	  return StatusCode::SUCCESS;
	}
    }

    ATH_MSG_ERROR("TagScaleFactor is not available on the jet, please make sure you called BoostedJetTaggers tag() function before calling this function.");
    return StatusCode::FAILURE;
}

StatusCode JetUncertaintiesTool::updateTagEfficiency(xAOD::Jet& jet, const double shift) const
{
  const bool TagScaleFactorwasAvailable  = m_accTagScaleFactor.isAvailable(jet);
  const xAOD::Jet& constJet = jet;
  if (TagScaleFactorwasAvailable)
    {
      if (!m_accTagScaleFactor.isAvailable(jet))
        {
          ATH_MSG_ERROR("TagScaleFactor was previously available but is not available on this jet.  This functionality is not supported.");
          return StatusCode::FAILURE;
        }
      const float value = m_accTagScaleFactor(constJet);
      if ( value < 1e-5 ) {
        // if the central SF is 0, we don't consider any uncertainties
        return StatusCode::SUCCESS;
      }
      if (m_accEffSF.isAvailable(jet)) {
        // if efficiency and efficiency SF are available, inefficiency SF will be calculated

        const float effSF = m_accEffSF(constJet);
        const float efficiency = m_accEfficiency(constJet);
        float sigeffSF = 1.0;
        float updated_efficiency = efficiency + shift; // efficiency value is varied

        if ( updated_efficiency < 1e-5 ) updated_efficiency=1e-5;
        if ( updated_efficiency > 1.0-1e-5 ) updated_efficiency=1.0-1e-5;
        m_accEfficiency(jet) = updated_efficiency;
        if (m_accSigeffSF.isAvailable(jet)) sigeffSF = m_accSigeffSF(constJet);

        const bool tagResult = m_accTagResult(constJet);
        if ( tagResult ) {
          // do nothing, since efficiency variation does not affect the tagged jets
          return StatusCode::SUCCESS;
        } else {
          // this jet is "failed"
	  // inefficiency SF will be recalculated for given uncertainty
          if ( std::abs(effSF - 1.0) < 1e-5 && std::abs(shift)>0 ) {
            // For other category, effSF=1.0. So efficiency variation cannot be propagated to the ineffSF i.e. (1 - eff)/(1 - eff) is always 1 not depending on eff value.
            // SF for signal (sigeffSF) is used instead of effSF when calculating it
            // Relative variation of ineffSF for signal is calculated here and used for the other category
	    float nominalIneffSFsig = (1. - sigeffSF*efficiency)/(1. - efficiency);
	    float variatedIneffSFsig = (1. - sigeffSF*updated_efficiency)/(1. - updated_efficiency);
	    m_accTagScaleFactor(jet) = variatedIneffSFsig/nominalIneffSFsig;
          } else {
            m_accTagScaleFactor(jet) = (1. - effSF*updated_efficiency) / (1. - updated_efficiency);
          }
          return StatusCode::SUCCESS;
        }
      } else {
        // if efficiency and efficiency SF are NOT available, inefficiency SF will not be calculated
        // do nothing
        return StatusCode::SUCCESS;
      }
    }

  ATH_MSG_ERROR("TagScaleFactor is not available on the jet, please make sure you called BoostedJetTaggers tag() function before calling this function.");
  return StatusCode::FAILURE;
}
