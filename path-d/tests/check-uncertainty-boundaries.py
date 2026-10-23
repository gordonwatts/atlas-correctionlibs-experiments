"""Direct untouched JetUncertaintiesTool oracle, including pseudo-data and statuses."""
import json,sys
from pathlib import Path
import numpy as np,ROOT
r=Path(__file__).resolve().parents[2];sys.path.insert(0,str(r/"path-d/build"))
from _declarative import GraphSet
ROOT.gSystem.Load("libJetUncertaintiesLib")
ROOT.gInterpreter.Declare("""
#ifndef XAOD_STANDALONE
#define XAOD_STANDALONE 1
#endif
#include "JetUncertainties/JetUncertaintiesTool.h"
#include "xAODJet/Jet.h"
#include "xAODEventInfo/EventInfo.h"
#include "PATInterfaces/SystematicSet.h"
struct UncertaintyReferenceProbe{
 JetUncertaintiesTool mc{"GraphReferenceMC"},pd{"GraphReferencePD"};
 UncertaintyReferenceProbe(){
  for(auto* tool:{&mc,&pd}){
   bool data=tool==&pd;
   if(tool->setProperty("JetDefinition",std::string("AntiKt4EMPFlow")).isFailure() ||
      tool->setProperty("MCType",std::string("MC23")).isFailure() ||
      tool->setProperty("ConfigFile",std::string("rel22/Winter2025_PreRec/R4_CategoryReduction_FullJER.config")).isFailure() ||
      tool->setProperty("IsData",data).isFailure() ||
      tool->setProperty("PseudoDataJERsmearingMode",data).isFailure() ||
      tool->initialize().isFailure())throw std::runtime_error("Oracle initialization failed");
  }
 }
 std::vector<double> evaluate(const std::string& name,const std::vector<double>& r){
  auto& tool=name.find("_PseudoData")!=std::string::npos?pd:mc;
  CP::SystematicSet sys(name=="NOSYS"?"":name);
  if(tool.applySystematicVariation(sys).isFailure())throw std::runtime_error("Oracle systematic failure");
  xAOD::Jet jet;jet.makePrivateStore();jet.setJetP4(xAOD::JetFourMom_t(r[0],r[1],r[2],r[3]));
  jet.setAttribute<char>("IsBjet",r[6]!=0);
  xAOD::EventInfo info;info.makePrivateStore();info.setAverageInteractionsPerCrossing(r[4]);info.auxdecor<float>("NPV")=r[5];
  auto status=tool.applyCorrection(jet,info);auto p=jet.jetP4();
  return {p.Pt(),p.Eta(),p.Phi(),p.M(),double(status.code())};
 }
};
""")
oracle=ROOT.UncertaintyReferenceProbe();engine=GraphSet(str(r/"path-d/payloads/graphs.json"))
rows=[]
for pt in [10.,20000.,60000.,1e6]:
 for eta in [-4.5,0.,2.5]:
  for phi in [0.,.3,-3.14159]:
   for b in [0.,1.]:rows.append(np.asarray([pt,eta,phi,1000.,30.,15.,b],dtype=np.float32).astype(float).tolist())
rows.append([20000.,0.,0.,1000.,30.,0.,0.])
checks=[];failures=[]
for name in engine.names():
 if name!="NOSYS" and not name.startswith("JET_"):continue
 bad=0;maxdelta=np.zeros(4)
 values=engine.batch(name,rows)
 for row,value in zip(rows,values):
  expected=np.asarray(oracle.evaluate(name,row));actual=np.asarray(value)
  delta=abs(actual[:4]-expected[:4]);delta[2]=abs((actual[2]-expected[2]+np.pi)%(2*np.pi)-np.pi)
  tolerance=[.01+2e-6*abs(expected[0]),2e-6,2e-6,.01+2e-6*abs(expected[3])]
  mapped=2 if actual[4]==0 else 1
  maxdelta=np.maximum(maxdelta,delta)
  if np.any(delta>tolerance) or not np.all(np.isfinite(delta)) or mapped!=expected[4]:
   bad+=1
   if len(failures)<30:failures.append(dict(name=name,row=row,expected=expected.tolist(),actual=actual.tolist()))
 checks.append(dict(systematic=name,cases=len(rows),mismatches=bad,max_abs=maxdelta.tolist()))
 print(name,bad,flush=True)
report=dict(cases=len(rows)*len(checks),systematics=len(checks),checks=checks,failures=failures,passed=all(c["mismatches"]==0 for c in checks))
(r/"path-d/results/uncertainty-boundaries.json").write_text(json.dumps(report,indent=2))
sys.exit(0 if report["passed"] else 1)
