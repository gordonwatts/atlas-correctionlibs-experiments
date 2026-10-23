# Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration

########################################################################
#                                                                      #
# JetCalibConfig: A helper module for configuring jet calibration      #
# The intention is to encode a limited set of configurations that      #
# are sane, to ease consistency for various users.                     #
# This module should eventually go in JetCalibTools and be kept in     #
# sync with the latest available recommendations.                      #
# Author: TJ Khoo                                                      #
#                                                                      #
########################################################################
from AthenaConfiguration.Enums import LHCPeriod
from JetRecConfig.StandardJetConstits import inputsFromContext

from AthenaCommon import Logging
jetcaliblog = Logging.logging.getLogger('JetCalibToolsConfig')

all = ['getJetCalibTool']

# These context definitions could be placed in another package and made
# more robust with a small python class to hold the info.
# The convention is: config file, calibarea, default calibration sequence
pflowcontexts = {
    "T0":("JES_MC15cRecommendation_PFlow_Aug2016_rel21.config","00-04-77","JetArea_Residual_EtaJES"),
    # Omit smearing, to avoid any efficiency loss
    "AnalysisLatest":("PreRec_R22_PFlow_ResPU_EtaJES_GSC_February23_230215.config","00-04-82","JetArea_Residual_EtaJES_GSC_Insitu"),
    "TrigLS2":("JES_MC16Recommendation_Consolidated_PFlow_Apr2019_Rel21_Trigger.config","00-04-82","JetArea_Residual_EtaJES_GSC_Insitu"),
    "Trigger":("JES_MC16Recommendation_Consolidated_PFlow_30May2022_Rel22_Trigger.config","00-04-82","JetArea_Residual_EtaJES_GSC_Insitu"),
    "TrigR22Prerec":("PreRec_R22_PFlow_ResPU_EtaJES_GSC_February23_230215.config", "00-04-82", "JetArea_Residual_EtaJES_GSC_Insitu"),
    "TrigHIUPC" : ("JES_MC16Recommendation_LowMu1718_MCJES_GSC_nTrkOn_PFlow_Sep2023_Rel21.config", "00-04-82", "EtaJES_GSC"),
}

topocontexts = {
    "T0":("JES_MC15cRecommendation_May2016_rel21.config","00-04-77","JetArea_Residual_EtaJES"),
    # Omit smearing, to avoid any efficiency loss
    "AnalysisLatest":("JES_MC16Recommendation_Consolidated_EMTopo_Apr2019_Rel21.config","00-04-82","JetArea_Residual_EtaJES_GSC_Insitu"),
    "TrigRun2":("JES_MC15cRecommendation_May2016_Trigger.config","00-04-77","JetArea_EtaJES_GSC_Insitu"),
    "TrigRun2GSC":("JES_data2016_data2015_Recommendation_Dec2016_rel21.config","00-04-77","JetArea_EtaJES_GSC_Insitu"),
    "TrigLS2":("JES_MC16Recommendation_Consolidated_EMTopo_Apr2019_Rel21_Trigger.config","00-04-82","JetArea_Residual_EtaJES_GSC_Insitu"),
    "Trigger":("JES_MC16Recommendation_Consolidated_EMTopo_Apr2019_Rel21_Trigger.config","00-04-82","JetArea_Residual_EtaJES_GSC_Insitu"),
    "HLLHC":("HLLHC/JES_MC16_HLLHC_Aug2021_rel21.config","00-04-82","JetArea_Residual_EtaJES"),
}

ufocontexts = {
    "T0":("Consolidated_R22_CSSKUFO_ResPU_EtaJES_GNNC_20231103.config", "00-04-83", "Residual_EtaJES_GNNC"),
}

rscanlc2 = {
    "RScanLatest":("JES_MC16Recommendation_Rscan2LC_22Feb2018_rel21.config","00-04-81","JetArea_Residual_EtaJES_GSC_Insitu")
}

rscanlc6 = {
    "RScanLatest":("JES_MC16Recommendation_Rscan6LC_22Feb2018_rel21.config","00-04-81","JetArea_Residual_EtaJES_GSC_Insitu")
}

fatjetcontexts = {
    "CombinedMass":  ("JES_MC16recommendation_FatJet_Trimmed_JMS_comb_17Oct2018.config","00-04-82","EtaJES_JMS"),
    "CaloMass":      ("JES_MC16recommendation_FatJet_Trimmed_JMS_calo_12Oct2018.config","00-04-82","EtaJES_JMS"),
    "TAMass":        ("JES_MC16recommendation_FatJet_Trimmed_JMS_TA_12Oct2018.config","00-04-82","EtaJES_JMS"),
    "SoftDrop":      ("JES_MC20PreRecommendation_R10_UFO_CSSK_SoftDrop_JMS_R21Insitu_26Nov2024.config","00-04-83","EtaJES_JMS"),
    "TrigUngroomed": ("JES_Full2012dataset_Rscan_June2014.config","00-04-77","JetArea_EtaJES"),
    "TrigTrimmed":   ("JES_MC15recommendation_FatJet_June2015_PtFrac4.config","00-04-82","EtaJES_JMS"),
    "TrigSoftDrop":  ("JES_MC16recommendation_R10_UFO_CSSK_SoftDrop_JMS_01April2020.config","00-04-82","EtaJES_JMS"),
    "TrigHLTSoftDrop":("JES_JMS_MC21_R10_PFlow_CSSK_SoftDrop_Dec2023_Trigger.config", "00-04-83","EtaJES_JMS"),
    "LargeRDNN":     ("JES_JMS_MC20dnnc_R10_UFO_CSSK_SoftDrop_Jun2023.config","00-04-83","LargeRDNN"),
}

# List AFII config files separately, to avoid needing to specify a different context
af2configs = {
    "AntiKt4EMPFlow": "JES_MC16Recommendation_AFII_PFlow_Apr2019_Rel21.config",
    "AntiKt4GPFlow": "JES_MC16Recommendation_AFII_PFlow_Apr2019_Rel21.config",
    "AntiKt4EMTopo":  "JES_MC16Recommendation_AFII_EMTopo_Apr2019_Rel21.config",
    "AntiKt4LCTopo":  "JES_MC16Recommendation_AFII_EMTopo_Apr2019_Rel21.config",
}

calibcontexts = {
    # Standard AntiKt4
    "AntiKt4EMPFlow":pflowcontexts,
    "AntiKt4EMPFlow_noElectrons":pflowcontexts,
    "AntiKt4EMPFlow_noMuons":pflowcontexts,
    "AntiKt4EMPFlow_noLeptons":pflowcontexts,
    "AntiKt4EMPFlow_tauSeedEleRM":pflowcontexts,
    "AntiKt4EMPFlowByVertex":pflowcontexts,
    "AntiKt4GPFlow":pflowcontexts,
    "AntiKt4UFOCSSK":ufocontexts,
    "AntiKt4EMTopo":topocontexts,
    "AntiKt4LCTopo":topocontexts,
    "AntiKt10LCTopo":fatjetcontexts,
    # Standard trimmed
    "AntiKt10LCTopoTrimmedPtFrac5SmallR20":fatjetcontexts,
    "AntiKt10LCTopoTrimmedPtFrac4SmallR20":fatjetcontexts,
    # Standard UFO CS+SK jets
    "AntiKt10UFOCSSKSoftDropBeta100Zcut10":fatjetcontexts,
    # UFO CS+SK jets with leptons removed
    "AntiKt10UFOCSSK_noElectronsSoftDropBeta100Zcut10":fatjetcontexts,
    "AntiKt10UFOCSSK_noMuonsSoftDropBeta100Zcut10":fatjetcontexts,
    "AntiKt10UFOCSSK_noLeptonsSoftDropBeta100Zcut10":fatjetcontexts,
    # Large-R PFlow Soft Drop CSSK
    "AntiKt10EMPFlowCSSKSoftDropBeta100Zcut10":fatjetcontexts,
    # R-Scan
    "AntiKt2LCTopo":rscanlc2,
    "AntiKt6LCTopo":rscanlc6,

}

hasInSitu = ["AntiKt4LCTopo", "AntiKt4EMTopo", "AntiKt4EMPFlow", "AntiKt4EMPFlow_noElectrons", "AntiKt4EMPFlow_noMuons", "AntiKt4EMPFlow_noLeptons","AntiKt4EMPFlow_tauSeedEleRM","TrigAntiKt4EMTopo"]

# This method extracts the relevant configuration, does some consistency checks,
# then forwards the configuration to defineJetCalibTool, returning the output.
# At present the interface allows for the calibseq to be chosen freely, other
# than checking that the data source is data for the in situ correction.
def getJetCalibTool(jetdef, context, data_type, calibseq = "", rhoname = "", pvname = "PrimaryVertices", gscdepth = "auto"):

    jetcollection = jetdef.basename

    # In principle we could autoconfigure
    if data_type not in ['data','mc','afii']:
        jetcaliblog.error("JetCalibConfig accepts data_type values: 'data', 'mc', 'afii'")
        raise ValueError("Unsupported data_type provided: '{0}".format(data_type))

    if jetcollection.endswith("Jets"):
        jetcaliblog.error("Jet collection '{0}'should be specified w/o 'Jets' in the name.".format(jetcollection))
        raise ValueError("Bad jet collection formatting in getJetCalibTool.")

    jetcaliblog.debug("Preparing calibration for {0}, in context {1} on sample type {2}".format(jetcollection,context,data_type))

    jetcontexts = calibcontexts[jetcollection]

    if jetcollection == "AntiKt4EMTopo" and context == "T0":
        if jetdef._cflags.GeoModel.Run >= LHCPeriod.Run4:
            context = "HLLHC"

    try:
        configfile, calibarea, calibseq_def = jetcontexts[context]        
        _calibseq = calibseq if calibseq else calibseq_def
        
        # Check that the calib sequence requests something sensible for the in situ calibration
        # Leave other checks for the tool code.
        # Might need to specialise if we decide MC trigger jets should also have in situ.
        if _calibseq.endswith("Insitu"):
            if data_type == 'data':
                if jetcollection not in hasInSitu:
                    raise ValueError("In situ calibration does not exist for {0}, context {1}".format(jetcollection,context))
            else:
                raise ValueError("In situ calibration requested for MC on {0}, context {1}".format(jetcollection,context))
        _configfile = configfile
        if data_type == "afii" and jetcollection in af2configs.keys():
            _configfile = af2configs[jetcollection]
        _data_type = data_type
        # Most likely an oversight, but R20/21 JetRecCalibrationFinder did not set the data flag
        # This affects the residual correction by scaling mu.
        # We should revert this later on, set up now for validation purposes
        if context == "T0":
            _data_type = "data"
        _pvname = ""
        if "Residual" in _calibseq or "GSC" in _calibseq and gscdepth!="EM3" or "LargeRDNN" in _calibseq:
            _pvname = pvname
        # HACK: For Trigger HI UPC who want to do pflow but avoid track-GSC
        if context == "TrigHIUPC":
            _pvname = pvname
        # HACK: For testing while we don't have finalised calibrations for trigger PF jets
        _jetcollection = jetcollection
        if "PFlow" in jetcollection and (context=="TrigSoftDrop" or context=="TrigHLTSoftDrop"):
            _jetcollection = jetcollection.replace("EMPFlow","UFO")
        if "_noElectrons" in jetcollection :
            _jetcollection = _jetcollection.replace("_noElectrons","")
        if "_noMuons" in jetcollection :
            _jetcollection = _jetcollection.replace("_noMuons","")
        if "_noLeptons" in jetcollection :
            _jetcollection = _jetcollection.replace("_noLeptons","")
        if "_tauSeedEleRM" in jetcollection :
            _jetcollection = _jetcollection.replace("_tauSeedEleRM","")

        if "ByVertex" in jetcollection:
            _jetcollection = jetcollection.replace("ByVertex","")
        return defineJetCalibTool(_jetcollection, context, _configfile, calibarea, _calibseq, _data_type, rhoname, _pvname, gscdepth)
    except KeyError as e:
        jetcaliblog.error("Context '{0}' not found for jet collection '{1}'".format(context,jetcollection))
        jetcaliblog.error("Options are '{0}".format(','.join(jetcontexts.keys())))
        raise e
    return None

# This method actually sets up the tool
def defineJetCalibTool(jetcollection, context, configfile, calibarea, calibseq, data_type, rhoname, pvname, gscdepth):
    # Abbreviate the calib sequence
    calibseqshort = ''.join([ step[0] for step in calibseq.split('_') ])
    toolname = "jetcalib_{0}_{1}_{2}".format(jetcollection,calibseqshort,context)
    #
    from AthenaConfiguration.ComponentFactory import CompFactory
    jct = CompFactory.JetCalibrationTool(toolname,
                                         JetCollection = jetcollection,
                                         ConfigFile = configfile,
                                         CalibArea = calibarea,
                                         CalibSequence = calibseq,
                                         IsData = (data_type == "data"),
                                         RhoKey = rhoname,
                                         PrimaryVerticesContainerName = pvname,
                                         GSCDepth = gscdepth
    )
    return jct

# This method extends the basic config getter to specify the requisite jet
# moments or other inputs
def getJetCalibToolPrereqs(modspec,jetdef):
    calibcontext, data_type, calibseq, rhoname, pvname, gscdepth = getCalibSpecsFromString(jetdef, modspec)
    if calibseq=="":
        cfg, calibarea, calibseq = calibcontexts[jetdef.basename][calibcontext]

    # For now, only dependent on calibseq -- can ignore Insitu, which is
    # added when getting the concrete tool
    prereqs = []
    prereqs.append("mod:ConstitFourMom")
    if "JetArea" in calibseq: # Will not insert a prefix here
        if calibcontext.startswith("Trig"): prereqs.append("input:HLT_EventDensity")
        elif pvname == "PrimaryVertices_initial": prereqs.append("input:EventDensityCustomVtxGNN")
        elif pvname != "PrimaryVertices": prereqs.append("input:EventDensityCustomVtx")
        else: prereqs.append(inputsFromContext("EventDensity")(jetdef))
    if "GSC" in calibseq:
        prereqs += ["mod:CaloEnergies"]
        if calibcontext != "TrigRun2": # No track/MS GSC for trigger w/o FTK
            prereqs += ["mod:TrackMoments",
                        "ghost:MuonSegment"]
    if "GNNC" in calibseq:
        prereqs += ["mod:CaloQuality","mod:TrackMoments"]
    if "CombinedMass" in calibcontext:
        prereqs += ["mod:TrackSumMoments"]
    if "LargeRDNN" in calibseq:
        prereqs += ["mod:CaloEnergiesLargeR","mod:ConstitFrac","mod:groomMRatio","mod:Width",
                    "mod:nsubjettiness","mod:nsubjettinessR","mod:ktsplitter","mod:ecorr",
                    "mod:ecorrR","mod:qw"]
    jetcaliblog.debug("Prereqs for calibseq '{0}': {1}".format(calibseq,str(prereqs)))
    return prereqs

# This method translates the mod specification string into calibration specifications
def getCalibSpecsFromString(jetdef, modspec):
    calibseq = ""
    # for the LCTopo EleRM context used for EleRM taus, we need alternative EventShape container
    rhoname = "EleRM_Kt4LCTopoOriginEventShape" if jetdef.context == "EleRM" else "auto"
    pvname = "PrimaryVertices"
    gscdepth = "auto"
    calibspecs = modspec.split(':')
    # Probably want data_type to come from elsewhere
    calibcontext, data_type = calibspecs[:2]
    if len(calibspecs)>2: calibseq = calibspecs[2]
    if len(calibspecs)>3: rhoname = calibspecs[3]
    if len(calibspecs)>4: pvname = calibspecs[4]
    if len(calibspecs)>5: gscdepth = calibspecs[5]

    return calibcontext, data_type, calibseq, rhoname, pvname, gscdepth

# This method instantiates the JetCalibTool given the input mod specification
def getJetCalibToolFromString(jetdef, modspec):
    calibcontext, data_type, calibseq, rhoname, pvname, gscdepth = getCalibSpecsFromString(jetdef, modspec)
    return getJetCalibTool(jetdef,calibcontext,data_type,calibseq,rhoname,pvname,gscdepth)
