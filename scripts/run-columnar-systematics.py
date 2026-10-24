"""Validate systematic kinematics from independently computed nominal columns."""
import argparse,json,sys
from pathlib import Path
import numpy as np,uproot,awkward as ak
r=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(r/"python"),str(r/"install/python")]
from atlas_correctionlib.systematics import Systematics
from atlas_correctionlib.kinematic_cache import fingerprint
p=argparse.ArgumentParser();p.add_argument("--events",type=int,default=100);p.add_argument("--reference",default="results/reference/reference.root");p.add_argument("--report",default="results/systematics-comparison.json");p.add_argument("--output");a=p.parse_args()
stored=np.load(r/"results/columnar-nominal.npz")
counts=stored["counts"][:a.events];n=int(counts.sum());p4=stored["values"][:n,8:12]
inputfile=Path("/cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt/src/AsgExternal/Asg_Test/cmt/TestFileLiteR3MC.txt").read_text().strip()
t=uproot.open(inputfile)["CollectionTree"]
fields=["EventInfoAuxDyn.averageInteractionsPerCrossing","PrimaryVerticesAuxDyn.trackParticleLinks","AnalysisJetsAuxDyn.PartonTruthLabelID","EventInfoAuxDyn.eventNumber"]
arr=t.arrays(fields,entry_stop=a.events,how=dict)
assert np.array_equal(ak.to_numpy(arr[fields[3]]),stored["eventNumber"][:a.events])
assert np.array_equal(ak.to_numpy(ak.num(arr[fields[2]])),counts)
mu=np.repeat(ak.to_numpy(arr[fields[0]]).astype(float),counts)
npv=np.repeat(ak.to_numpy(ak.sum(ak.num(arr[fields[1]],axis=2)>1,axis=1)).astype(float),counts)
bjet=ak.to_numpy(ak.flatten(arr[fields[2]]))==5
ref=uproot.open(r/a.reference)["analysis"]
engine=Systematics(r/"payloads");checks={};seen=[]
writer=uproot.recreate(r/a.output,compression=uproot.LZ4(1)) if a.output else None
for name,out in engine.variations(p4,mu,npv,bjet):
 seen.append(name);summary={}
 if writer is not None:
  data={coord:ak.unflatten(out[:,i],counts) for i,coord in enumerate(["pt","eta","phi","m"])}
  data["eventNumber"]=stored["eventNumber"][:len(counts)]
  data["originalJetIndex"]=ak.local_index(data["pt"],axis=1)
  data["status"]=ak.unflatten(np.full(n,2,dtype=np.int8),counts)
  writer[name]=data
 for i,coord in enumerate(["pt","eta","phi","m"]):
  truth=ak.to_numpy(ak.flatten(ref["check_"+coord+"_"+name].array(entry_stop=a.events)))
  delta=np.abs(out[:,i]-truth)
  if coord=="phi":delta=np.abs((out[:,i]-truth+np.pi)%(2*np.pi)-np.pi)
  tol=2e-6 if coord in ("eta","phi") else .01+2e-6*np.abs(truth)
  bad=(delta>tol)|~np.isfinite(delta)
  summary[coord]=dict(max_abs=float(delta.max(initial=0)),mismatches=int(bad.sum()),first_bad=np.flatnonzero(bad)[:5].tolist())
 checks[name]=summary
 print(name,sum(x["mismatches"] for x in summary.values()),flush=True)
if writer is not None:
 writer["__kinematic_cache__"]=json.dumps(dict(version=1,fingerprint=fingerprint(r/"payloads",p4=p4,mu=mu,npv=npv,bjet=bjet,counts=counts,events=stored["eventNumber"][:len(counts)]),names=engine.engine.names()))
 writer.close()
expected={k.removeprefix("check_pt_") for k in ref.keys() if k.startswith("check_pt_")}
report=dict(scope="kinematics-only-prototype",events=len(counts),jets=n,systematics=len(seen),missing=sorted(expected-set(seen)),unexpected=sorted(set(seen)-expected),checks=checks)
report["passed"]=not(report["missing"] or report["unexpected"]) and all(v["mismatches"]==0 for s in checks.values() for v in s.values())
(r/a.report).write_text(json.dumps(report,indent=2))
sys.exit(0 if report["passed"] else 1)
