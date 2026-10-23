"""Check every kinematic and JVT variation on the complete input under reordering and workers."""
import json,sys
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import numpy as np,correctionlib
r=Path(__file__).resolve().parents[1];sys.path[:0]=[str(r/"install/python"),str(r/"python")]
from _atlas_corrections import JetSystematics
from atlas_correctionlib.jvt import Jvt
d=np.load(r/"results/jvt-inputs.npz");p4=d["p4"];n=len(p4)
rows=np.column_stack([p4,d["mu"],d["npv"],d["bjet"]])
engine=JetSystematics(str(r/"payloads/uncertainty-tables.json.gz"),str(r/"payloads/uncertainty-manifest.json"))
tables=correctionlib.CorrectionSet.from_file(str(r/"payloads/uncertainty-tables.json.gz"));jvt=Jvt(r/"payloads",tables)
order=np.random.default_rng(9241).permutation(n);checks=[]
def evaluate(name,idx):
 sigma=(1 if name.endswith("up") else -1) if "NNJvtEfficiency" in name else 0
 kin=np.asarray(engine.batch("NOSYS" if sigma else name,rows[idx].tolist()))
 score,passed,sf,valid=jvt.evaluate(kin[:,:4],d["sumpt"][idx],d["jvfcorr"][idx],d["haspv"][idx],d["ishs"][idx],sigma)
 return idx,np.column_stack([kin,score,passed,sf,valid])
for name in engine.names()+["JET_NNJvtEfficiency__1up","JET_NNJvtEfficiency__1down"]:
 _,expected=evaluate(name,np.arange(n))
 for chunk,workers in [(1021,1),(4096,4)]:
  indices=[order[i:i+chunk] for i in range(0,n,chunk)];out=np.empty_like(expected)
  with ThreadPoolExecutor(max_workers=workers) as pool:
   for idx,values in pool.map(lambda idx:evaluate(name,idx),indices):out[idx]=values
  bad=int(np.count_nonzero(out!=expected))
  checks.append(dict(systematic=name,chunk=chunk,workers=workers,mismatches=bad,max_abs=float(np.max(abs(out-expected)))))
 print(name,checks[-2]["mismatches"],checks[-1]["mismatches"],flush=True)
report=dict(events=len(d["counts"]),jets=n,systematics=len(engine.names())+2,checks=checks,passed=all(x["mismatches"]==0 for x in checks))
(r/"results/systematic-determinism.json").write_text(json.dumps(report,indent=2));sys.exit(0 if report["passed"] else 1)
