"""Whole graph pipeline vs diagnostic stage calls for all states, plus nominal invariance."""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import sys,json
import numpy as np
r=Path(__file__).resolve().parents[2];sys.path[:0]=[str(r/"path-d/build"),str(r/"path-d/python")]
from _declarative import GraphSet
engine=GraphSet(str(r/"path-d/payloads/graphs.json"))
raw=np.load(r/"path-d/results/columnar-inputs.npz")["rows"]
d=np.load(r/"path-d/results/jvt-inputs.npz")
n=len(raw);nom=np.asarray(engine.batch("nominal",raw.tolist()))
saved=np.load(r/"path-d/results/columnar-nominal.npz")["values"]
assert np.array_equal(nom,saved)
order=np.random.default_rng(9241).permutation(n);checks=[]
for chunk,workers in [(1021,1),(4096,4)]:
 chunks=[order[i:i+chunk] for i in range(0,n,chunk)];actual=np.empty_like(nom)
 def evaluate(idx):return idx,np.asarray(engine.batch("nominal",raw[idx].tolist()))
 with ThreadPoolExecutor(max_workers=workers) as pool:
  for idx,values in pool.map(evaluate,chunks):actual[idx]=values
 checks.append(dict(kind="nominal_invariance",jets=n,chunk=chunk,workers=workers,mismatches=int(np.count_nonzero(actual!=nom))))
idx=order[:1024]
rawrows=raw[idx]
for name in engine.names():
 if not name.startswith("pipeline_"):continue
 state=name.removeprefix("pipeline_")
 sigma=(1 if state.endswith("up") else -1) if "NNJvtEfficiency" in state else 0
 kinname="NOSYS" if sigma else state
 kin=np.asarray(engine.batch(kinname,np.column_stack([nom[idx,8:12],d["mu"][idx],d["npv"][idx],d["bjet"][idx]]).tolist()))
 weight=np.asarray(engine.batch("jvt_"+str(sigma),np.column_stack([kin[:,:2],d["sumpt"][idx],d["jvfcorr"][idx],d["haspv"][idx],d["ishs"][idx]]).tolist()))
 pipeline=np.column_stack([rawrows,d["npv"][idx],d["bjet"][idx],d["sumpt"][idx],d["jvfcorr"][idx],d["haspv"][idx],d["ishs"][idx]])
 result=np.asarray(engine.batch(name,pipeline.tolist()))
 expected=np.column_stack([nom[idx],kin,weight])
 checks.append(dict(kind="pipeline_composition",state=state,jets=len(idx),mismatches=int(np.count_nonzero(result!=expected))))
print("Pipeline and nominal checks",len(checks))
report=dict(passed=all(x["mismatches"]==0 for x in checks),checks=checks)
(r/"path-d/results/pipelines.json").write_text(json.dumps(report,indent=2))
assert report["passed"]
