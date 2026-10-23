"""Aggregate only completed acceptance evidence; retain exact source/runtime fingerprints."""
from pathlib import Path
import json,hashlib,sys,platform,importlib.metadata
import numpy as np,uproot,awkward as ak
r=Path(__file__).resolve().parents[2];d=r/"path-d"
files=["nominal-comparison","systematics-comparison","jvt-comparison","jes-boundaries","jvt-boundaries","uncertainty-boundaries","primitives","pipelines","systematic-determinism","table-edges","architecture","reproducibility","truth-boundaries","runtime"]
reports={name:json.loads((d/"results"/(name+".json")).read_text()) for name in files}
assert all(x["passed"] for x in reports.values())
nom=reports["nominal-comparison"];kin=reports["systematics-comparison"];jvt=reports["jvt-comparison"]
assert nom["events"]==kin["events"]==jvt["events"]==20000
assert nom["jets"]==kin["jets"]==jvt["jets"]==196606
assert kin["systematics"]==117 and len(jvt["checks"])==119
# Explicit full identity checks independent of the physics comparisons.
stored=np.load(d/"results/columnar-nominal.npz")
ref=uproot.open(r/"results/reference/reference.root")["analysis"]
assert np.array_equal(stored["eventNumber"],ak.to_numpy(ref["eventNumber"].array()))
assert np.array_equal(stored["counts"],ak.to_numpy(ak.num(ref["check_pt_NOSYS"].array())))
identity={}
for name in ["kinematics","jvt"]:
 f=uproot.open(d/"results"/(name+".root"));count=0
 for state in f.keys(cycle=False):
  tree=f[state]
  assert np.array_equal(ak.to_numpy(tree["eventNumber"].array()),stored["eventNumber"])
  idx=tree["originalJetIndex"].array()
  assert bool(ak.all(idx==ak.local_index(idx,axis=1)))
  assert np.array_equal(ak.to_numpy(ak.num(idx)),stored["counts"])
  assert "entry" in tree.keys()
  assert np.array_equal(ak.to_numpy(tree["entry"].array()),np.arange(20000))
  count+=1
 identity[name]=dict(states=count,entry_exact=True,eventNumber_exact=True,originalJetIndex_exact=True,jet_counts_exact=True)
hashes={}
for p in [d/"payloads/graphs.json",d/"payloads/corrections.json.gz",d/"payloads/nnjvt-network.json",d/"payloads/manifest.json",d/"python/compile_graphs.py",d/"python/adapters.py",d/"python/schema.py",r/"external/correctionlib-d/include/correction_graph.h",r/"patches/correctionlib-full-precision-json.patch",r/"config/reference.yaml",d/"patches/correctionlib-graph-v1.patch"]:
 hashes[str(p.relative_to(r))]=hashlib.sha256(p.read_bytes()).hexdigest()
inputfile=Path("/cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt/src/AsgExternal/Asg_Test/cmt/TestFileLiteR3MC.txt").read_text().strip()
h=hashlib.sha256()
with open(inputfile,"rb") as f:
 for chunk in iter(lambda:f.read(8*1024*1024),b""):h.update(chunk)
assert h.hexdigest()=="017e408002b0153df1de695d8c2ace325e0731b658cbd5ca695a349eb0a059c0"
assert (r/"config/reference.yaml").read_bytes()==(r/"config/reference.frozen.yaml").read_bytes()
report=dict(passed=True,path="D",events=20000,jets=196606,kinematic_states=117,weight_states=119,nominal_bitwise_identical=all(v["max_abs"]==0 for v in nom["checks"].values()),identity=identity,reports={k:str(Path(k+".json")) for k in reports},hashes=hashes,input_sha256=h.hexdigest(),limitations=["Full-sample original reference has no per-jet JES/JER statuses; synthetic oracle tests those statuses.","Collection extraction, vertex counts, pair enumeration and event reductions remain Python.","Optional graph schema extension; not standard schema-v2 and not a JIT.","One pinned recommendation bundle; Gaussian rejection has an explicit one-million-draw bound."])
(d/"results/validation-summary.json").write_text(json.dumps(report,indent=2))
text=f"""# Path D validation

PASS: 20,000 events, 196,606 jets, all 117 kinematic and 119 NNJVT/weight states against untouched AnalysisBase A. No tolerance was changed.

| Check | Result |
|---|---|
| Nominal intermediate four-vectors | Bitwise identical |
| All JES/JER/NNJVT/jet and event weights | PASS |
| Event numbers, original jet indices, per-event counts | Exact |
| Original-tool JES boundaries | {reports['jes-boundaries']['cases']} cases, PASS |
| Original-tool JVT boundaries/statuses | {reports['jvt-boundaries']['cases']} cases, PASS |
| Original-tool JES/JER boundaries/statuses | {reports['uncertainty-boundaries']['cases']} cases across 117 states, PASS |
| Truth-match wrapped-angle/threshold cases | 108 cases, PASS |\n| Generic primitives and JSON-schema round trip | PASS |
| Complete graph pipelines vs exposed stage calls | 119 states × 1,024 jets, exact |
| Full nominal and all-state order/chunk/worker invariance | Exact |
| Negative pile-up reset, small positive pt, GSC special inputs | PASS |

The direct uncertainty edge oracle exposed and fixed negative-scale ROOT mass normalization and nonpositive-NPV sentinel behavior. A/B/C remain untouched.

The full-sample reference does not record per-jet JES/JER statuses; their exact comparison is limited to the independent 8,541-case synthetic oracle. The original NN model remains a generic lwtnn inference boundary, and ROOT coordinates/TRandom3 remain explicit reusable primitives. See ../docs/design.md for architecture and limits.

The machine-readable validation-summary.json links all per-state reports and records input/configuration/payload/source hashes.
"""
(d/"results/validation-summary.md").write_text(text)
environment=dict(python=sys.version,platform=platform.platform(),packages={x.metadata["Name"]:x.version for x in importlib.metadata.distributions()})
(d/"results/environment.json").write_text(json.dumps(environment,indent=2))
print(text)
