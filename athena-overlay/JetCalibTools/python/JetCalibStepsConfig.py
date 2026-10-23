# Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
## *********************************************
from AnaAlgorithm.DualUseConfig import isAthena
if not isAthena:
    ## If in AnalysisBase and not in Athena, import JetAnalysisCommon
    ## which will set-up replacement for some Athena modules and allow standard jet config to be invoked.
    ## IMPORTANT:  1st import (and define) AlgSequence
    import AnaAlgorithm.AlgSequence # noqa: F401
    import JetRecConfig.JetAnalysisCommon # noqa: F401
## *********************************************
from AthenaCommon import Logging
jcslog = Logging.logging.getLogger('JetCalibStepsConfig')

from JetToolHelpers.HelperConfig import VarToolCfg, HistoInputCfg
from AthenaConfiguration.ComponentFactory import CompFactory
from AthenaConfiguration.AutoConfigFlags import GetFileMD
from AthenaConfiguration.Enums import LHCPeriod
from PathResolver import PathResolver
import json

def smearingStep(flags, **configDict):
    """ Configuration of the Smearing step. """

    configDict.setdefault('OutScale', 'JetSmearedMomentum')

    # HistoReaderMC and HistoReaderData can be specified simultaneously using a single "HistoReader" YAML block.
    # This should contain the common configuration for both and histNameMC/histNameData entries for the histogram names.
    # This cannot be used simultaneously with a HistoReaderMC or HistoReaderData block
    if "HistoReader" in configDict:
        if "HistoReaderMC" in configDict:
            raise JetCalibConfigError("Both HistoReader and HistoReaderMC blocks included in YAML config")
        if "HistoReaderData" in configDict:
            raise JetCalibConfigError("Both HistoReader and HistoReaderData blocks included in YAML config")
        
        # Build the HistoReaderMC and HistoReaderData blocks from the HistoReader block
        histoReader = configDict.pop("HistoReader")
        histNameMC = histoReader.pop("histNameMC")
        histNameData = histoReader.pop("histNameData")
        configDict["HistoReaderMC"] = dict(histoReader)
        configDict["HistoReaderData"] = dict(histoReader)
        configDict["HistoReaderMC"]["histName"] = histNameMC
        configDict["HistoReaderData"]["histName"] = histNameData

    configDict["HistoReaderMC"]["inputFile"] = PathResolver.FindCalibFile(configDict["HistoReaderMC"]["inputFile"])
    configDict["HistoReaderData"]["inputFile"] = PathResolver.FindCalibFile(configDict["HistoReaderData"]["inputFile"])

    histToolMC = HistoInputCfg(flags, "HistToolMC", **configDict["HistoReaderMC"])
    histToolData = HistoInputCfg(flags, "HistToolData", **configDict["HistoReaderData"])
    configDict["HistoReaderMC"] = histToolMC
    configDict["HistoReaderData"] = histToolData

    smearStep = CompFactory.SmearingCalibStep("SmearingCalibStep", **configDict)

    return [smearStep]

def puresidualStep(flags, **configDict):
    configDict.setdefault('OutScale', 'JetPileupScaleMomentum')
    configDict.setdefault('IsData', not flags.Input.isMC)
    PU_step = CompFactory.Pileup1DResidualCalibStep("PUResid", **configDict)
    return [PU_step]

def newpuresidualStep(flags, **configDict):
    configDict.setdefault('OutScale', 'JetPileupScaleMomentum')
    configDict.setdefault('IsData', not flags.Input.isMC)

    histoParamsMu = configDict.pop('histoParamsMu')
    inputFile = PathResolver.FindCalibFile(configDict.pop('calibFile'))
    histoParamsMu['inputFile'] = inputFile

    histoParamsNPV = configDict.pop('histoParamsNPV')
    histoParamsNPV['inputFile'] = inputFile

    histToolMu = HistoInputCfg(flags, "HistToolMu", **histoParamsMu)
    histToolNPV = HistoInputCfg(flags, "HistToolNPV", **histoParamsNPV)
    configDict["histTool_mu"] = histToolMu
    configDict["histTool_NPV"] = histToolNPV

    PU_step = CompFactory.PileupAreaResidualCalibStep("PUAreaResidual", **configDict)
    return [PU_step]

def gscStep(flags, **configDict):

    configDict.setdefault('OutScale', 'JetGSCScaleMomentum')

    defaultFileGSC = PathResolver.FindCalibFile(configDict.pop('fileGSC'))

    # These HistoInput2D defaults can't be set in the C++, so are set here:
    defaultHistTools = dict(
        histTool_EM3 = [dict(varX = "pt", varY = "EM3", histName=f"AntiKt4EMPFlow_EM3_interpolation_resp_eta_{j}", inputFile=defaultFileGSC) for j in range(35)],
        histTool_CharFrac = [dict(varX = "pt", varY = "ChargedFraction", histName = f"AntiKt4EMPFlow_chargedFraction_interpolation_resp_eta_{j}", inputFile=defaultFileGSC) for j in range(25)],
        histTool_Tile0 = [dict(varX = "pt", varY = "Tile0", histName=f"AntiKt4EMPFlow_Tile0_interpolation_resp_eta_{j}", inputFile=defaultFileGSC) for j in range(18)],
        histTool_nTrk=[dict(varX = "pt", varY = dict(Name="nTrk", Type="int",), histName=f"AntiKt4EMPFlow_nTrk_interpolation_resp_eta_{j}", inputFile=defaultFileGSC) for j in range(25)],
        histTool_trackWIDTH=[dict(varX = "pt", varY = "trackWIDTH", histName=f"AntiKt4EMPFlow_trackWIDTH_interpolation_resp_eta_{j}", inputFile=defaultFileGSC) for j in range(25)],
        histTool_PunchThrough=[dict(varX = "e", varY = dict(Name="Nsegments", Type="int",), histName=f"AntiKt4EMPFlow_PunchThrough_interpolation_resp_eta_{j}", inputFile=defaultFileGSC) for j in range(2)],
    )

    gsc_steps = ['histTool_EM3', 'histTool_CharFrac', 'histTool_Tile0', 'histTool_nTrk', 'histTool_trackWIDTH']
    if configDict.get('applyPunchThrough',False):
        gsc_steps.append('histTool_PunchThrough')
        
    # Build the hist tools
    for key in gsc_steps:
        # Use defaultHistTools by default
        if key not in configDict:
            toolArray = defaultHistTools[key]

        # In this case the full list of sub-tools has been specified in the YAML
        # and the defaultDict will be fully overwritten (ie. all parameters must be specified)
        elif isinstance(configDict[key],list):
            toolArray = configDict[key]
            for subDict in toolArray:
                subDict.setdefault('inputFile',defaultFileGSC)

        # Functionality to build the arrays of histogram reader from a shorter block in the YAML file
        # Fall back on defaultHistTools for defaults
        else:
            baseDict = dict(configDict[key])
            N_hist = baseDict.pop('N_hist')
            histNameBase = baseDict.pop('histNameBase')
            inputFile = baseDict.pop('inputFile', defaultFileGSC)
            varX = baseDict.pop('varX',defaultHistTools[key][0]['varX'])
            varY = baseDict.pop('varY', defaultHistTools[key][0]['varY'])
            toolArray = [dict(varX = varX, varY = varY, histName=f'{histNameBase}_{j}', inputFile = inputFile) for j in range(N_hist)]

        # Convert array of properties to HistoInput tools
        configDict[key] = [HistoInputCfg(flags, Tname=f"{key.split('_')[1]}_{j}", **toolConfig) for j, toolConfig in enumerate(toolArray)]

    GSCstep = CompFactory.GSCCalibStep("gsccalibstep", **configDict)

    return [GSCstep]

def etajesStep(flags, **configDic):
    configDic.setdefault('OutScale', 'JetEtaJESScaleMomentum')
    pVars = configDic.pop("ParametrizedVars")

    jesstep = CompFactory.EtaJESCalibStep("EtaJESCalib",
                                              VarToolE= VarToolCfg(flags,  var=pVars['varE']),
                                              VarToolEta= VarToolCfg(flags, var=pVars["varEta"]),
                                              **configDic
                                              )
    return [jesstep]

def jmsStep(flags, **configDic):

    configDic.setdefault('OutScale', 'JetJMSScaleMomentum')
    histoParams = configDic.pop('histoParams')
    histoParams['inputFile'] = PathResolver.FindCalibFile(configDic.pop('HistoFile'))

    configDic["histoReaderJMS"] = HistoInputCfg(flags, "HistToolJMS", **histoParams)
    configDic['varToolX'] = VarToolCfg(flags, var=histoParams['varX'], Tname="VarToolX_JMS")
    configDic['varToolZ'] = VarToolCfg(flags, var=histoParams['varZ'], Tname="VarToolZ_JMS")

    jmsstep = CompFactory.JMSCalibStep("JMSCalib",
                                       **configDic
                                       )
    return [jmsstep]

def insituStep(flags, **configDic):
    configDic.setdefault('OutScale', 'JetInsituScaleMomentum')
    histEtaInterCalib = configDic.pop('histEtaInterCalib')
    histAbsCalib = configDic.pop('histAbsCalib')

    histAbsJMSCalib = configDic.pop('JMS',None)

    histoReaderEta_vec, histoReaderAbs_vec = [], []

    for infile in configDic.pop('fileInsitu'):
        histoReaderEta_vec.append(dict(inputFile = PathResolver.FindCalibFile(infile), **histEtaInterCalib))
        histoReaderAbs_vec.append(dict(inputFile = PathResolver.FindCalibFile(infile), **histAbsCalib))

    configDic['HistoReaderEtaInter'] = [HistoInputCfg(flags, "HistToolEtaInter"+str(j), **etaDic) for j, etaDic in enumerate(histoReaderEta_vec)]
    configDic['HistoReaderAbs'] = [HistoInputCfg(flags, "HistToolAbs"+str(j), **absDic) for j, absDic in enumerate(histoReaderAbs_vec)]

    configDic['isMC'] = flags.Input.isMC

    insituSteps = [CompFactory.InSituCalibStep("insitucalibstep", **configDic)]

    # JMS
    if histAbsJMSCalib:
        histAbsJMSCalib['inputFile'] = PathResolver.FindCalibFile(histAbsJMSCalib['inputFile'])
        insituSteps.append(
            CompFactory.InSituJMSCalibStep("insitujmscalibstep", 
                CalibrateMC = configDic.get("CalibrateMC",False),
                isMC = flags.Input.isMC,
                # modifying insitu scale rather than defining a new scale
                InScale = "JetInsituScaleMomentum",
                OutScale = "JetInsituScaleMomentum",
                HistoReaderAbsJMS = HistoInputCfg(flags, "HistoToolAbsJMS", **histAbsJMSCalib),
                ))

    return insituSteps

def af3Step(flags, **configDic):
    configDic.setdefault('OutScale','JetFastSimScaleMomentum')
    # Get the settings for the histograms:
    histoParams = configDic.pop('histoParams')
    histoParams['inputFile'] = PathResolver.FindCalibFile(configDic.pop('CalibConstantFile'))
    configDic["histoTool"] = HistoInputCfg(flags, "histoTool", **histoParams)

    return [CompFactory.Generic4VecCorrectionStep("AF3", **configDic)]

def ptResidualStep(flags, **configDic):
    configDic.setdefault('OutScale','JetPtResidualScaleMomentum')
    # Get the settings for the histograms:
    histoParams = configDic.pop('histoParams')

    # Define varTool to switch to bin centers
    if configDic['useBinCenter']:
        varYHisto = histoParams.pop('varYHisto')
        configDic['varTool'] = VarToolCfg(flags, var=varYHisto, Tname="VarTool_for_binCenter")

    histoParams['inputFile'] = PathResolver.FindCalibFile(configDic.pop('CalibConstantFile'))
    # 2D histogram with correction factors
    configDic["histoTool"] = HistoInputCfg(flags, "histoTool", **histoParams)

    return [CompFactory.Generic4VecCorrectionStep("PtResidual", **configDic)]

def mc2mcStep(flags, **configDic):
    configDic.setdefault('OutScale','JetMC2MCScaleMomentum')
    # Generator and version are the first item
    for key, value in flags.Input.GeneratorsInfo.items():
        generator = key
        generator_version = value
        break

    # Get the shower model:
    showerModel = ''
    # Check first if the DSID is on the exceptions list
    mcDSID = flags.Input.MCChannelNumber
    with open(PathResolver.FindCalibFile("JetCalibTools/MC2MC_exceptions_DSID.json")) as read_file:
        data = json.load(read_file)
        for key, value in data.items():
            if key == mcDSID:
                showerModel = value

    if showerModel == '':
        genType, psType, hadType = generatorDic[generator]
        version = generator_version.replace('.','')[:3]
        if (generator == 'Pythia8' or generator == 'Pythia8B') and not version.startswith('8'):
            version = '8'+version
        showerModel = genType+"-"+version+"-"+psType+"-"+hadType

        with open(PathResolver.FindCalibFile("JetCalibTools/MC2MC_showerRemap.json")) as read_file:
            data = json.load(read_file)
            foundMatch = False
            for key, value in data.items():
                if key == showerModel:
                    showerModel = value
                    foundMatch = True
                    break
            if not foundMatch:
                for key, value in data.items():
                    if key == genType+"-"+version:
                        showerModel = value+"-"+psType+"-"+hadType
                        break

    jcslog.info(f'Using shower model {showerModel} for the MC-to-MC correction')

    # Get the settings for the histograms:
    baseHistoParams = configDic.pop('histoParams')
    baseHistoParams['inputFile'] = PathResolver.FindCalibFile(configDic.pop('CalibConstantFileName')+'_'+showerModel+'.root')

    histNameBase = baseHistoParams.pop('histNameBase')
    for flav in configDic.pop('flavours'):
        if flav == 'c':
            configDic['doCjetCorrection'] = True
        elif flav == 'b':
            configDic['doBjetCorrection'] = True
        histoParams = dict(varX = baseHistoParams['varX'],varY = baseHistoParams['varY'],
                           histName=f'{histNameBase}_{flav}',
                           inputFile=baseHistoParams['inputFile'])
        configDic['mc2mcHist_'+flav] = HistoInputCfg(flags,Tname='HistoTool_MC2MC_'+flav,**histoParams)

    configDic['isMC2MCCorr'] = True

    return [CompFactory.Generic4VecCorrectionStep("MC2MC", **configDic)]

#####################
    
calibStepDic = dict(
    JetArea = None,
    Residual = puresidualStep,
    ResidualNew = newpuresidualStep,
    EtaJES = etajesStep,
    JMS = jmsStep,
    GSC = gscStep,
    Insitu = insituStep,
    Smear = smearingStep,
    AF3 = af3Step,
    PtResidual = ptResidualStep,
    MC2MC = mc2mcStep,

)

#####################
generatorDic = {
    "Herwigpp": ["Herwigpp", "angular", "cluster"],
    "Herwig7": ["Herwig", "angular", "cluster"],
    "Sherpa": ["Sherpa", "dipole", "cluster"],
    "Pythia8B": ["PythiaB", "dipole", "cluster"],
    "Pythia8": ["Pythia", "dipole", "cluster"]
}

def calibConfigToToolList(flags, calibSeqOverride=None, **configDict):
    """
    Returns a list of instantiated tools for each of the calibration steps. 
    Tools are instantiated by calling functions declared in the calibStepDic dictionary.
    The order of the steps is determined by the Sequence block of the config. 
    The calibSeqOverride argument can be set to a '_'-separated string of step names, 
    which will override the step ordering set by the Sequence block.
    """

    # Identify type of sample
    isFullSim = True
    if flags.Input.isMC:
        metaData = GetFileMD(flags.Input.Files[0])
        simFlavour = metaData.get('Simulator','') # ATLFAST3 or FullG4
        if 'ATLFAST3' in simFlavour:
            isFullSim = False
            sampleKey = 'AF3'
        else:
            sampleKey = 'FullSim'
    else:
        sampleKey = 'Data'

    if flags.GeoModel.Run == LHCPeriod.Run2:
        runKey = 'Run2'
    elif flags.GeoModel.Run == LHCPeriod.Run3:
        runKey = 'Run3'
    elif flags.GeoModel.Run >= LHCPeriod.Run4:
        runKey = 'Run4'
    else:
        jcslog.warning('LHCPeriod not recognised')


    if calibSeqOverride:
        # If calibSeqOverride is set, use this to determine ordering
        sequence = calibSeqOverride.split('_')
        jcslog.info('Expert option calibSeqOverride set - overriding step sequence')
    else:
        # Ordering of calib steps based on Sequence block and type of sample
        try:
            seqDict = configDict.pop('Sequence')
            sequence = seqDict[runKey][sampleKey]
        except KeyError:
            raise JetCalibConfigError(f"{runKey} {sampleKey} sample identified. YAML should specify step ordering via the following block structure: \n \
    Sequence:\n \
        {runKey}: \n \
            {sampleKey}: [list of steps] ")

    toolList = []
    jcslog.debug('Configuring jet calib steps:')
    for step in sequence:

        if step not in configDict:
            raise JetCalibConfigError(f'Sequence includes step {step} but no YAML block is provided.')
        configDict.get(step).pop('prereqs',{}) # removes the 'prereqs' entry not refined in steps    

        # expert option to skip a step
        if configDict.get(step).pop('noRun',False):
            jcslog.warning(f'Expert option: Skipping calib step {step}')
            continue

        # Warning: Insitu for MC
        if step=="Insitu" and flags.Input.isMC and not configDict.get("Insitu").get("CalibrateMC",False):
            jcslog.warning('Insitu step included for MC but CalibrateMC is False - no calibration will be run')

        if step=="MC2MC":
            # Print warning if running MC2MC calibration for data
            if not flags.Input.isMC:
                jcslog.warning('Running MC2MC calibration for data')

            # Skip MC to MC calibration for Pythia8
            for key, value in flags.Input.GeneratorsInfo.items():
                generator = key
                break
            if 'Pythia' in generator:
                jcslog.debug('Skipping MC2MC calibration for Pythia8')
                continue

        # Warning if running AF3 calibration for data or FullSim
        if step=="AF3":
            if not flags.Input.isMC:
                jcslog.warning('Running FastSimulation calibration for data')

            if isFullSim:
                jcslog.warning('Running FastSimulation calibration for full sim')

        calibFunc = calibStepDic.get(step,None)

        if calibFunc is None:
            raise NotImplementedError(f'Calibration step {step} is not found in calibStepDic')

        calibConfig = configDict.get(step)

        # Config can contain run-specific settings in a 'RunX:' sub-block
        for overrideKey in ['Run2', 'Run3', 'Run4']:
            # All override blocks should be removed from the configDict
            overrideDict = calibConfig.pop(overrideKey,{}) 
            # Only apply the overrides for the relevant Run
            if runKey!=overrideKey or not overrideDict:
                continue
            jcslog.debug(f'{step}: Applying {runKey} override settings')

            for key in overrideDict:
                if key in calibConfig:
                    jcslog.warning(f'{key} will be overwritten by {overrideKey} settings')
                calibConfig[key] = overrideDict[key]

        # Start from ConstitScale. For subsequent steps set InScale to OutScale of previous step
        if len(toolList)==0:
            inScale = 'JetConstitScaleMomentum'
        else:
            inScale = toolList[-1].OutScale

        calibConfig.setdefault('InScale', inScale)

        if calibConfig['InScale']!=inScale:
            jcslog.warning(f'InScale set to {calibConfig['InScale']} in YAML config, but expected {inScale} from Sequence ordering -- is this intentional?')
        
        # each func returns a list (to allow one YAML block to configure multiple steps run in order)
        newToolList = calibFunc(flags, **calibConfig)
        jcslog.debug(f'{step}: InScale = {newToolList[0].InScale}, OutScale = {newToolList[-1].OutScale}')

        toolList += newToolList

    return toolList

def calibToolFromConfigFile(flags, configFile, name = "jetcalib", calibSeqOverride = None):
    """
    Returns a list of instantiated tools for each of the calibration steps. 
    The order of the steps is determined by the Sequence block of the config, unless calibSeqOverride is set.

    Parameters:
    -----------
    configFile: str
        Path to YAML configuration file
    name: str
        Internal name of the configured jet calib tool
    
    Expert options:
    ---------------
    calibSeqOverride: str
        Optional '_'-separated string of step names. If provided this will override the step ordering set by the Sequence block.
    """
    infoMsg = f'Configuring JetCalibTools with {configFile}'
    jcslog.info(infoMsg)

    configDic = load_yaml_cfg(configFile)

    globalSettings = configDic.pop('Global',{})

    calibTool = CompFactory.JetCalibTool(name, CalibSteps=calibConfigToToolList(flags, calibSeqOverride, **configDic), **globalSettings)
    return calibTool

def load_yaml_cfg(configFile):
    from yaml import safe_load

    path_configFile = PathResolver.FindCalibFile(configFile)
    configDic = safe_load(open(path_configFile))
    return configDic


class JetCalibConfigError(Exception):
    """ Exception raised for invalid jet calibration config """

    def __init__(self, message):
        super().__init__(message)
        self.message = message

    def __str__(self):
        return self.message
