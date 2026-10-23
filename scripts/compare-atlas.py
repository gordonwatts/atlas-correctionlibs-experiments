"""Compare untouched and replacement AnalysisBase outputs, including full branch coverage."""
import argparse,json,sys
from pathlib import Path
import numpy as np,awkward as ak,uproot
r=Path(__file__).resolve().parents[1]
ap=argparse.ArgumentParser();ap.add_argument("--reference",default="results/reference/reference.root");ap.add_argument("--candidate",default="results/replacement/replacement.root");ap.add_argument("--report",default="results/atlas-comparison.json");args=ap.parse_args()
a=uproot.open(r/args.reference)["analysis"]
b=uproot.open(r/args.candidate)["analysis"]
n=b.num_entries
checks={};missing=sorted(set(a.keys())-set(b.keys()));unexpected=sorted(set(b.keys())-set(a.keys()))
for key in sorted(set(a.keys())&set(b.keys())):
 x=a[key].array(entry_stop=n);y=b[key].array()
 if ak.to_list(ak.num(x,axis=0))!=ak.to_list(ak.num(y,axis=0)):raise ValueError(key)
 if x.ndim>1:
  if not ak.all(ak.num(x)==ak.num(y)):raise ValueError(("jet counts",key))
  x=ak.flatten(x);y=ak.flatten(y)
 x=ak.to_numpy(x);y=ak.to_numpy(y)
 d=np.abs(y.astype(np.float64)-x.astype(np.float64))
 if "phi" in key:d=np.abs((y.astype(np.float64)-x.astype(np.float64)+np.pi)%(2*np.pi)-np.pi)
 if x.dtype.kind in "biu" or "select" in key or key in ("eventNumber","runNumber"):tol=0
 elif "eta" in key or "phi" in key:tol=2e-6
 elif "NNJvt" in key or "Efficiency" in key or "weight_" in key:tol=1e-7+2e-6*np.abs(x)
 else:tol=.01+2e-6*np.abs(x)
 bad=(x!=y) if np.isscalar(tol) and tol==0 else ((d>tol)|~np.isfinite(d))
 checks[key]=dict(max_abs=float(d.max(initial=0)),mismatches=int(bad.sum()),first_bad=np.flatnonzero(bad)[:5].tolist())
report=dict(events=n,branches=len(checks),missing=missing,unexpected=unexpected,checks=checks,passed=not(missing or unexpected) and all(v["mismatches"]==0 for v in checks.values()))
(r/args.report).write_text(json.dumps(report,indent=2))
print("A/B events:",n,"branches:",len(checks),"passed:",report["passed"])
for k,v in checks.items():
 if v["mismatches"]:print(k,v)
sys.exit(0 if report["passed"] else 1)
