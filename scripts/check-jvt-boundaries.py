"""Independent original-tool oracle for JVT weight/status boundaries."""
import sys,json
from pathlib import Path
import numpy as np,ROOT
r=Path(__file__).resolve().parents[1];sys.path.insert(0,str(r/"install/python"))
from _atlas_corrections import JvtEfficiency
ROOT.gSystem.Load("libJetJvtEfficiencyLib")
ROOT.gInterpreter.Declare("""
#ifndef XAOD_STANDALONE
#define XAOD_STANDALONE 1
#endif
#include "JetJvtEfficiency/NNJvtEfficiencyTool.h"
#include "xAODJet/Jet.h"
#include "PATInterfaces/SystematicSet.h"
#include "PATInterfaces/SystematicVariation.h"
struct JvtReferenceProbe{
 CP::NNJvtEfficiencyTool tool{"ReferenceBoundaryTool"};
 JvtReferenceProbe(){
  if(tool.setProperty("WorkingPoint",std::string("FixedEffPt")).isFailure() ||
     tool.setProperty("SFFile",std::string("JetJvtEfficiency/May2024/NNJvtSFFile_Run3_EMPFlow.root")).isFailure() ||
     tool.initialize().isFailure())throw std::runtime_error("Reference initialization failed");
 }
 std::vector<double> evaluate(double pt,double eta,bool pass,bool hs,int sigma){
  CP::SystematicSet sys;
  if(sigma)sys.insert(CP::SystematicVariation("JET_NNJvtEfficiency",sigma));
  if(tool.applySystematicVariation(sys).isFailure())throw std::runtime_error("Reference systematic failed");
  xAOD::Jet jet;jet.makePrivateStore();jet.setJetP4(xAOD::JetFourMom_t(pt,eta,.3,1000));
  jet.setAttribute<char>("isJvtHS",hs);
  float sf=0;auto status=pass?tool.getEfficiencyScaleFactor(jet,sf):tool.getInefficiencyScaleFactor(jet,sf);
  return {double(sf),double(status.code())};
 }
};
""")
oracle=ROOT.JvtReferenceProbe();engine=JvtEfficiency(str(r/"payloads/uncertainty-tables.json.gz"),str(r/"payloads/uncertainty-manifest.json"))
pts=[0.,19999.,20000.,20001.,59999.,60000.,60001.,100000.]
etas=[-3.,-2.5,-2.499999,0.,2.499999,2.5,3.]
count=0;failures=[]
for pt in pts:
 for eta in etas:
  for passing in [False,True]:
   for hs in [False,True]:
    for sigma in [-1,0,1]:
     expected=list(oracle.evaluate(pt,eta,passing,hs,sigma))
     value=engine.batch([[pt,eta,float(passing),float(hs)]],sigma)[0]
     mapped=2 if value[1]==0 else 1
     if abs(value[0]-expected[0])>1e-7+2e-6*abs(expected[0]) or mapped!=expected[1]:
      failures.append(dict(pt=pt,eta=eta,passing=passing,hs=hs,sigma=sigma,reference=expected,actual=value))
     count+=1
report=dict(cases=count,failures=failures,passed=not failures)
(r/"results/jvt-boundaries.json").write_text(json.dumps(report,indent=2))
print("JVT reference boundary cases:",count,"failures:",len(failures))
sys.exit(0 if report["passed"] else 1)
