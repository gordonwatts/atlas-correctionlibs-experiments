"""Consolidate independently produced validation reports; fail on incomplete acceptance."""
from pathlib import Path
import hashlib,json,datetime
import numpy as np,awkward as ak,uproot
r=Path(__file__).resolve().parents[1]
def sha(p):
 h=hashlib.sha256()
 with Path(p).open("rb") as f:
  for block in iter(lambda:f.read(8*1024*1024),b""):h.update(block)
 return h.hexdigest()
names=["nominal-comparison","atlas-comparison","systematics-comparison","jvt-comparison","bc-kinematics-comparison","bc-jvt-comparison","determinism","systematic-determinism","phys-comparison","jes-boundaries","jvt-boundaries"]
reports={n:json.loads((r/"results"/(n+".json")).read_text()) for n in names}
for n,x in reports.items():
 assert x["passed"],n
 if "comparison" in n:assert x["events"]==(100 if n=="phys-comparison" else 20000),n
assert reports["systematics-comparison"]["systematics"]==117
assert len(reports["jvt-comparison"]["checks"])==119
assert reports["systematic-determinism"]["systematics"]==119
assert reports["systematic-determinism"]["events"]==20000
assert reports["jes-boundaries"]["cases"]==110
assert reports["jvt-boundaries"]["cases"]==672
assert (r/"config/reference.yaml").read_bytes()==(r/"config/reference.frozen.yaml").read_bytes()
testlist=Path("/cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt/src/AsgExternal/Asg_Test/cmt/TestFileLiteR3MC.txt")
inputpath=Path(testlist.read_text().strip())
with uproot.open(inputpath) as f:
 t=f["CollectionTree"];assert t.num_entries==20000
 events=t["EventInfoAuxDyn.eventNumber"].array()
 counts=ak.num(t["AnalysisJetsAuxDyn.DetectorEta"].array())
assert int(ak.sum(counts))==196606
for name in ["reference/reference.root","replacement/replacement.root"]:
 with uproot.open(r/"results"/name) as f:
  t=f["analysis"];assert t.num_entries==20000
  assert ak.all(t["eventNumber"].array()==events),name
columnar={}
for name,expected in [("columnar-kinematics.root",117),("columnar-jvt.root",119)]:
 with uproot.open(r/"results"/name) as f:
  keys=f.keys(cycle=False);assert len(keys)==expected,(name,len(keys))
  for key in keys:
   t=f[key];assert t.num_entries==20000
   assert ak.all(t["eventNumber"].array()==events),(name,key)
   ids=t["originalJetIndex"].array()
   assert ak.all(ak.num(ids)==counts),(name,key)
   assert ak.all(ids==ak.local_index(ids)),(name,key)
  columnar[name]=dict(states=len(keys),events=20000,identity_and_order_exact=True,sha256=sha(r/"results"/name))
athena=(r/"logs/athena-smoke.log").read_text()
assert "CorrectionLib Athena adapters initialized successfully: 5 tools" in athena
assert "Application Manager Terminated successfully" in athena
assert "6 passed" in (r/"logs/tests.log").read_text()
assert "1 success, 0 failure" in (r/"logs/bundle-only.log").read_text()
bundle=json.loads((r/"payloads/bundle-manifest.json").read_text())
for name,info in bundle["files"].items():assert sha(r/"payloads"/name)==info["sha256"],name
report_hashes={n:sha(r/"results"/(n+".json")) for n in names}
summary=dict(passed=True,created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
 scope="Run-3 FullSim AntiKt4EMPFlow, Category JES / Full JER, pseudo-data JER, NNJVT FixedEffPt",
 input=dict(path=str(inputpath),sha256=sha(inputpath),size=inputpath.stat().st_size,events=20000,jets=196606),
 configuration_sha256=sha(r/"config/reference.yaml"),
 environment=json.loads((r/"config/environment.json").read_text()),
 bundle_manifest_sha256=sha(r/"payloads/bundle-manifest.json"),
 reports={n:dict(path="results/"+n+".json",sha256=report_hashes[n],passed=True) for n in names},
 columnar_artifacts=columnar,
 per_systematic={n:reports[n]["checks"] for n in ["systematics-comparison","jvt-comparison","bc-kinematics-comparison","bc-jvt-comparison"]},
 auxiliary=dict(unit_tests=6,jes_boundary_cases=110,jvt_boundary_cases=672,athena_release="25.0.76",athena_initialized_tools=5,athena_events=0,bundle_only_events=1),
 performance=json.loads((r/"results/performance.json").read_text()),
 limitations=["Validation applies to this pinned configuration and sample; unsupported bundles fail explicitly.",
 "Athena validation is build and initialization only; AnalysisBase is the numerical authority.",
 "Timing is a component benchmark with possibly warm caches; feature reconstruction is not timed.",
 "Compiler manifests describe conversion-stage status; this report records subsequent end-to-end validation.",
 "The bundle-only smoke restricts DATAPATH; no system-call file-open audit was performed."])
(r/"results/validation-summary.json").write_text(json.dumps(summary,indent=2))
lines=["# Validation report","","PASS: the pinned demo meets the original numerical tolerances on all 20,000 events (196,606 jets).",
"","| Comparison | Coverage | Result |","|---|---|---|",
"| A vs B | 1,190 branches; unchanged analysis configuration | PASS |",
"| A vs C | 117 nominal/JES/JER states and 119 NNJVT/weight states | PASS |",
"| B vs C | Same complete state coverage | PASS |",
"| Nominal calibration stages | Pile-up, eta/JES, GSC: all coordinates | Bitwise identical |",
"| Shuffled input, chunk sizes and workers | All 119 states, full input | Exactly invariant |",
"| PHYS A vs B | 100 events, 1,190 branches | PASS |",
"| Boundary tests | 110 eta/JES and 672 JVT cases | PASS |",
"| Unit tests | 6 tests | PASS |",
"| Athena 25.0.76 | Build and initialize five tools, zero events | PASS |",
"","## Reproduction and evidence","","Run scripts/run-demo.sh inside atlas_al9. See ../README.md for individual commands.",
"Full per-systematic differences and mismatch counts are in validation-summary.json and the linked reports.",
"The input SHA256 is "+summary["input"]["sha256"]+".",
"Configuration SHA256: "+summary["configuration_sha256"]+".",
"","The CorrectionLib v2.7.0 base requires the included full-precision JSON parsing patch. No tolerance was relaxed.",
"","## Limits",""]+["- "+x for x in summary["limitations"]]+["","## Reports",""]+["- ["+n+"]("+n+".json)" for n in names]
(r/"results/validation-summary.md").write_text("\n".join(lines)+"\n")
print("PASS: consolidated full-sample acceptance, identity, provenance and runtime bundle verification.")
