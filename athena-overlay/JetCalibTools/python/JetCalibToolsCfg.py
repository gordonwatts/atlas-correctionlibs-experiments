# Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration

from JetRecConfig.StandardJetConstits import inputsFromContext
from PathResolver import PathResolver

from AthenaCommon import Logging
jetcaliblog = Logging.logging.getLogger('JetCalibToolsConfig')

from AthenaConfiguration.Enums import LHCPeriod

all = ['getJetCalibTool']

commonPath = '/eos/atlas/atlascerngroupdisk/perf-jets/JSV/JetCalibToolsMigration/configFiles/'

calibdic_T0 = {
    "AntiKt4EMPFlow": commonPath+"T0/EMPFlow/JES_MC15cRecommendation_PFlow_Aug2016_rel21.yaml",
    "AntiKt4EMTopo":  commonPath+"T0/EMTopo/JES_MC15cRecommendation_May2016_rel21.yaml",
    "AntiKt4LCTopo":  commonPath+"T0/LCTopo/JES_MC15cRecommendation_May2016_rel21.yaml",
    "AntiKt10UFOCSSKSoftDropBeta100Zcut10": commonPath+"LatestRecommendations/largeR_Run23/JES_MC20PreRecommendation_R10_UFO_CSSK_SoftDrop_JMS_R21Insitu_26Nov2024.yaml",
}

calibdic_analysis_Run2 = {
    "AntiKt4EMPFlow": commonPath+"LatestRecommendations/smallR_mc20_Run2/PreRec_R22_PFlow_ResPU_EtaJES_GSC_February23_230215.yaml",
    "AntiKt4EMTopo":  commonPath+"LatestRecommendations/EMTopo/PreRec_R22_EMTopo_ResPU_EtaJES_October23_231024.yaml",
    "AntiKt10UFOCSSKSoftDropBeta100Zcut10": commonPath+"LatestRecommendations/largeR_Run23/JES_MC20PreRecommendation_R10_UFO_CSSK_SoftDrop_JMS_R21Insitu_26Nov2024.yaml",
}

calibdic_analysis_Run3 = {
    "AntiKt4EMPFlow": commonPath+"LatestRecommendations/smallR_mc23_Run3/AntiKt4EMPFlow_MC23a_PreRecR22_Phase2_CalibConfig_ResPU_EtaJES_GSC_241208_InSitu.yaml",
    "AntiKt4EMTopo":  commonPath+"LatestRecommendations/EMTopo/PreRec_R22_EMTopo_ResPU_EtaJES_October23_231024.yaml",
    "AntiKt10UFOCSSKSoftDropBeta100Zcut10": commonPath+"LatestRecommendations/largeR_Run23/JES_MC20PreRecommendation_R10_UFO_CSSK_SoftDrop_JMS_R21Insitu_26Nov2024.yaml",
}

calibdic = {
    "T0": calibdic_T0,
    "Run2": calibdic_analysis_Run2,
    "Run3": calibdic_analysis_Run3,
}

# This method actually sets up the tool
def defineJetCalibTool(jetdef, modspec):
    from JetCalibTools.JetCalibStepsConfig import calibToolFromConfigFile

    # Get the yaml file and calibration sequence
    cfg, calibSeqOverride = getJetCalibToolSettings(jetdef, modspec)
    path_configFile = PathResolver.FindCalibFile(cfg) 
    toolname = "jetcalib_new_{0}_{1}".format(jetdef.basename,modspec)
    jct = calibToolFromConfigFile(jetdef._cflags, path_configFile, toolname, calibSeqOverride)

    return jct

# This method extends the basic config getter to specify the requisite jet
# moments or other inputs
def getJetCalibToolPrereqs(jetdef, modspec):
    from JetCalibTools.JetCalibStepsConfig import load_yaml_cfg

    cfg, calibSeqOverride = getJetCalibToolSettings(jetdef, modspec)
    configDic = load_yaml_cfg(cfg)

    prereqs = ["mod:ConstitFourMom"]
    pvname = "PrimaryVertices" # this can be set dinamically in future

    for step, step_config in configDic.items():
        # JetArea
        if step_config.get("DoJetArea", False):
            if modspec.startswith("Trig"):
                prereqs.append("input:HLT_EventDensity")
            elif pvname == "PrimaryVertices_initial":
                prereqs.append("input:EventDensityCustomVtxGNN")
            elif pvname != "PrimaryVertices":
                prereqs.append("input:EventDensityCustomVtx")
            else:
                prereqs.append(inputsFromContext("EventDensity")(jetdef))

        # read prereqs from context or default config block
        prereq_block = step_config.get("prereqs", {})
        step_prereqs = prereq_block.get(modspec, prereq_block.get("default", []))
        prereqs.extend(step_prereqs)

    # remove duplication and keep order
    seen = set()
    prereqs_unique = []
    for p in prereqs:
        if p not in seen:
            prereqs_unique.append(p)
            seen.add(p)

    return prereqs_unique

# Get specific settings for JetCalibTools
def getJetCalibToolSettings(jetdef, modspec):

    calibspecs = modspec.split(':')
    context = calibspecs[0]

    ##############################
    # Get the jet collection name
    ##############################

    # For some specific jet collections, e.g. lepton-free PFlow jets,
    # we want to apply the calibrations of the default jet PFlow jets

    jetcollection = jetdef.basename

    if "_noElectrons" in jetcollection:
        jetcollection = jetcollection.replace("_noElectrons","")
    if "_noMuons" in jetcollection:
        jetcollection = jetcollection.replace("_noMuons","")
    if "_noLeptons" in jetcollection :
        jetcollection = jetcollection.replace("_noLeptons","")
    if "_tauSeedEleRM" in jetcollection :
        jetcollection = jetcollection.replace("_tauSeedEleRM","")

    ##############################
    # Get the calibration sequence
    ##############################

    # Per default, the calibration sequence is determined from the yaml file
    calibSeqOverride = None
    # Check if specified in configuration (e.g. for low / no pT jet collections)
    if len(calibspecs) > 2:
        calibSeqOverride = calibspecs[2]
    # Check if T0 configuration, if yes, want to apply only certain calibrations
    elif context == "T0":
        calibSeqOverride = "JetArea_Residual_EtaJES"
        if jetcollection == "AntiKt10UFOCSSKSoftDropBeta100Zcut10":
            calibSeqOverride = "EtaJES_JMS"

    ##########################################
    # Retrieve the yaml file for JetCalibTools
    ##########################################
    if context == "AnalysisLatest":
        if jetdef._cflags.GeoModel.Run == LHCPeriod.Run2:
            cfg = calibdic["Run2"][jetcollection]
        elif jetdef._cflags.GeoModel.Run == LHCPeriod.Run3:
            cfg = calibdic["Run3"][jetcollection]
        elif jetdef._cflags.GeoModel.Run >= LHCPeriod.Run4:
            cfg = calibdic["HLLHC"][jetcollection]
    else:
        cfg = calibdic[context][jetcollection]

    return cfg, calibSeqOverride
