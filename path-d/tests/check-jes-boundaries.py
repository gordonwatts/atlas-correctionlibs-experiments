"""Original EtaJESCorrection oracle: low-energy extrapolation, eta edges and high-energy freezing."""
import sys,json
from pathlib import Path
import numpy as np,ROOT
r=Path(__file__).resolve().parents[2];sys.path.insert(0,str(r/"path-d/build"))
from _declarative import GraphSet
ROOT.gSystem.Load("libJetCalibToolsLib")
ROOT.gInterpreter.Declare("""
#ifndef XAOD_STANDALONE
#define XAOD_STANDALONE 1
#endif
#include "JetCalibTools/CalibrationMethods/EtaJESCorrection.h"
#include "JetCalibTools/JetEventInfo.h"
#include "xAODJet/Jet.h"
#include "TEnv.h"
struct JESReferenceProbe{
 TEnv env;
 std::unique_ptr<EtaJESCorrection> tool;
 JESReferenceProbe(const std::string& path){
  if(env.ReadFile(path.c_str(),kEnvLocal))throw std::runtime_error("Reference settings failed");
  tool=std::make_unique<EtaJESCorrection>("JESBoundaryTool",&env,"AntiKt4EMPFlow","CalibArea-00-04-83/",false,false);
  if(tool->initialize().isFailure())throw std::runtime_error("Reference EtaJES initialization failed");
 }
 std::vector<double> evaluate(double pt,double eta,double phi,double mass){
  xAOD::Jet jet;jet.makePrivateStore();xAOD::JetFourMom_t p{float(pt),float(eta),float(phi),float(mass)};
  jet.setJetP4(p);jet.setAttribute<xAOD::JetFourMom_t>("JetPileupScaleMomentum",p);
  jet.setAttribute<float>("DetectorEta",float(eta));JetEventInfo info;
  if(tool->calibrate(jet,info).isFailure())throw std::runtime_error("Reference EtaJES calculation failed");
  auto q=jet.getAttribute<xAOD::JetFourMom_t>("JetEtaJESScaleMomentum");
  return {q.Pt(),q.Eta(),q.Phi(),q.M()};
 }
};
""")
cfg=r/"payloads/JetCalibTools/CalibArea-00-04-83/CalibrationConfigs/AntiKt4EMPFlow_MC23a_PreRecR22_Phase2_CalibConfig_ResPU_EtaJES_GSC_241208_InSitu.config"
oracle=ROOT.JESReferenceProbe(str(cfg));s=json.loads((r/"payloads/nominal-manifest.json").read_text())["settings"]
engine=GraphSet(str(r/"path-d/payloads/graphs.json"))
failures=[];count=0;maxdelta=np.zeros(4)
for eta in [-4.5,-4.4999,-3.2,-1.2,-.1,0.,.1,1.2,2.5,4.4999,4.5]:
 for pt in [1.,10.,500.,9999.,14999.,15000.,15001.,20000.,1e6,1e9]:
  p=np.asarray([pt,eta,.3,1000.],dtype=np.float32).astype(float)
  expected=np.asarray(oracle.evaluate(*p))
  row=[*p,p[1],0.,0.,s["OffsetCorrection.DefaultMuRef"]/s["MuScaleFactor"],s["OffsetCorrection.DefaultNPVRef"],0.,-1.,0.,0.,0.]
  actual=np.asarray(engine.evaluate("nominal_prepared",row))[4:8];delta=abs(actual-expected);maxdelta=np.maximum(maxdelta,delta)
  tol=np.asarray([.01+2e-6*abs(expected[0]),2e-6,2e-6,.01+2e-6*abs(expected[3])])
  if np.any(delta>tol) or not np.all(np.isfinite(delta)):failures.append(dict(input=p.tolist(),reference=expected.tolist(),actual=actual.tolist()))
  count+=1
report=dict(cases=count,maximum_absolute_differences=maxdelta.tolist(),failures=failures,passed=not failures)
(r/"path-d/results/jes-boundaries.json").write_text(json.dumps(report,indent=2));print("JES boundary cases:",count,"failures:",len(failures))
sys.exit(0 if report["passed"] else 1)
