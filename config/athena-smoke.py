from AthenaConfiguration.AllConfigFlags import initConfigFlags
from AthenaConfiguration.MainServicesConfig import MainServicesCfg
from AthenaConfiguration.ComponentFactory import CompFactory
flags=initConfigFlags()
flags.Input.Files=[]
flags.Input.isMC=True
flags.Exec.MaxEvents=0
flags.lock()
cfg=MainServicesCfg(flags)
tools=[
 CompFactory.JetCalibrationTool("CorrectionLibCalib",JetCollection="AntiKt4EMPFlow",CalibSequence="JetArea_Residual_EtaJES_GSC",ConfigFile="AntiKt4EMPFlow_MC23a_PreRecR22_Phase2_CalibConfig_ResPU_EtaJES_GSC_241208_InSitu.config",CalibArea="00-04-83",IsData=False),
 CompFactory.JetUncertaintiesTool("CorrectionLibJES",JetDefinition="AntiKt4EMPFlow",MCType="MC23",ConfigFile="rel22/Winter2025_PreRec/R4_CategoryReduction_FullJER.config",IsData=False),
 CompFactory.JetUncertaintiesTool("CorrectionLibPseudoData",JetDefinition="AntiKt4EMPFlow",MCType="MC23",ConfigFile="rel22/Winter2025_PreRec/R4_CategoryReduction_FullJER.config",IsData=True,PseudoDataJERsmearingMode=True),
 CompFactory.CP.NNJvtEfficiencyTool("CorrectionLibJvtEfficiency",JetContainer="AnalysisJets",WorkingPoint="FixedEffPt",SFFile="JetJvtEfficiency/May2024/NNJvtSFFile_Run3_EMPFlow.root"),
 CompFactory.CP.NNJvtSelectionTool("CorrectionLibJvtSelection",JetContainer="AnalysisJets",WorkingPoint="FixedEffPt"),
]
for tool in tools:cfg.addPublicTool(tool)
cfg.addEventAlgo(CompFactory.CorrectionLibSmoke("CorrectionLibSmoke",Tools=tools))
status=cfg.run(0)
raise SystemExit(0 if status.isSuccess() else 1)
