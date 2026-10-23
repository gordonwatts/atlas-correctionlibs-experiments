"""Compare graph truth-pair decisions to the original xAOD p4/DeltaR expression."""
import json,sys
from pathlib import Path
import numpy as np,ROOT
r=Path(__file__).resolve().parents[2];sys.path.insert(0,str(r/"path-d/build"))
from _declarative import GraphSet
ROOT.gInterpreter.Declare("""
#ifndef XAOD_STANDALONE
#define XAOD_STANDALONE 1
#endif
#include "xAODJet/Jet.h"
bool graphTruthOracle(float eta,float phi,float truthpt,float trutheta,float truthphi){
 xAOD::Jet jet,truth;jet.makePrivateStore();truth.makePrivateStore();
 jet.setJetP4(xAOD::JetFourMom_t(30000.,eta,phi,1000.));
 truth.setJetP4(xAOD::JetFourMom_t(truthpt,trutheta,truthphi,1000.));
 float dr=jet.p4().DeltaR(truth.p4());
 return dr<float(.3) && truth.pt()>10000.;
}
""")
engine=GraphSet(str(r/"path-d/payloads/graphs.json"));rows=[]
for eta in [0.,2.5,-4.5]:
 for delta in [0.,float(np.nextafter(np.float32(.3),np.float32(0))),float(np.float32(.3)),float(np.nextafter(np.float32(.3),np.float32(1)))]:
  for phi,tphi in [(0.,0.),(3.14159,-3.14159),(-3.14159,3.14159)]:
   for pt in [9999.,10000.,10001.]:
    rows.append(np.asarray([eta,phi,pt,eta+delta,tphi],dtype=np.float32).astype(float).tolist())
expected=[bool(ROOT.graphTruthOracle(*row)) for row in rows]
actual=[bool(v[0]) for v in engine.batch("truth_match",rows)]
failures=[dict(row=row,expected=x,actual=y) for row,x,y in zip(rows,expected,actual) if x!=y]
report=dict(passed=not failures,cases=len(rows),failures=failures)
(r/"path-d/results/truth-boundaries.json").write_text(json.dumps(report,indent=2))
print(report);assert report["passed"]
