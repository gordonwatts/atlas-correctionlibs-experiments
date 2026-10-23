"""Synthetic payload checks through the actual graph/correction-call evaluator."""
from pathlib import Path
import json,tempfile,sys
import numpy as np
r=Path(__file__).resolve().parents[2];sys.path.insert(0,str(r/"path-d/build"))
from _declarative import GraphSet
bundle=json.loads((r/"path-d/payloads/graphs.json").read_text())
def graph(name,inputs):
 return dict(inputs=inputs,steps=[dict(op="call",inputs=inputs,outputs=["result"],correction=name)],outputs=["result"])
bundle["graphs"]["edge_pileup"]=graph("pileup_scale",["pt","area","rho","offset"])
for f in ["chargedFraction","Tile0","EM3","nTrk","trackWIDTH"]:
 bundle["graphs"]["edge_"+f]=graph("gsc_"+f,["pt","eta","scratch","feature"])
with tempfile.NamedTemporaryFile(mode="w",suffix=".json",dir=r/"path-d/payloads") as tmp:
 json.dump(bundle,tmp);tmp.flush();engine=GraphSet(tmp.name)
 values=engine.batch("edge_pileup",[[100.,1.,90.,20.],[100.,1.,90.,9.],[100.,1.,90.,10.]])
 assert values[0][0]*100==10
 assert abs(values[1][0]*100-1)<1e-14
 assert values[2][0]==0
 for f in ["chargedFraction","EM3","nTrk","trackWIDTH"]:
  assert engine.batch("edge_"+f,[[50.,.5,0.,0.],[50.,.5,0.,-1.]])==[[1.],[1.]]
 assert engine.evaluate("edge_Tile0",[50.,.5,0.,-1.])==[1.]
report=dict(passed=True,pileup_cases=3,gsc_special_cases=9)
(r/"path-d/results/table-edges.json").write_text(json.dumps(report,indent=2))
print(report)
