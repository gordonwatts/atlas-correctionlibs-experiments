import json
from pathlib import Path
import uproot
root=Path(__file__).resolve().parents[1]
f=uproot.open(root/"results/reference/reference.root")
t=f["analysis"]
branches=t.keys()
systs=sorted(b.removeprefix("check_pt_") for b in branches if b.startswith("check_pt_"))
report=dict(entries=t.num_entries,systematics=systs,n_systematics=len(systs),branches=branches)
(root/"results/reference/coverage.json").write_text(json.dumps(report,indent=2))
print(json.dumps({k:v for k,v in report.items() if k!="branches"},indent=2))
assert len(systs)>1,"No systematic variations in reference"
assert any("PseudoData" in s for s in systs),"Missing pseudo-data JER"
assert any("jvt" in b.lower() and "1up" in b.lower() for b in branches),"Missing JVT variations"
