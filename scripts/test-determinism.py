"""Full-input reordering, chunking, and worker invariance of the shared evaluator."""
import sys,json
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import numpy as np
r=Path(__file__).resolve().parents[1];sys.path.insert(0,str(r/"install/python"))
from _atlas_corrections import Nominal
s=json.loads((r/"payloads/nominal-manifest.json").read_text())["settings"]
engine=Nominal(str(r/"payloads/nominal.json.gz"),s["OffsetCorrection.DefaultMuRef"],s["OffsetCorrection.DefaultNPVRef"],s["MuScaleFactor"])
inputs=np.load(r/"results/columnar-inputs.npz")["rows"];expected=np.load(r/"results/columnar-nominal.npz")["values"]
rng=np.random.default_rng(72536);order=rng.permutation(len(inputs));checks=[]
for chunksize,workers in [(257,1),(4096,4)]:
 chunks=[order[i:i+chunksize] for i in range(0,len(order),chunksize)]
 result=np.empty_like(expected)
 def evaluate(indices):return indices,np.asarray(engine.batch(inputs[indices].tolist()))
 with ThreadPoolExecutor(max_workers=workers) as pool:
  for indices,values in pool.map(evaluate,chunks):result[indices]=values
 checks.append(dict(chunk_size=chunksize,workers=workers,jets=len(inputs),mismatches=int(np.count_nonzero(result!=expected)),max_abs=float(np.max(abs(result-expected)))))
report=dict(scope="nominal-full-input",checks=checks,passed=all(v["mismatches"]==0 for v in checks))
(r/"results/determinism.json").write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
sys.exit(0 if report["passed"] else 1)
