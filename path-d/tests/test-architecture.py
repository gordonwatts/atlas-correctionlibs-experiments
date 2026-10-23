from pathlib import Path
from collections import Counter
import json
r=Path(__file__).resolve().parents[2];d=r/"path-d"
source=(r/"external/correctionlib-d/include/correction_graph.h").read_text()
for forbidden in ["BJES_Response","Flavor_Composition","Pileup_Offset","JET_","NNJvt","JetCalibrationTool","JetUncertaintiesTool","backend.hxx","systematics.hxx","efficiency.hxx"]:
 assert forbidden not in source,forbidden
adapter=(d/"python/adapters.py").read_text()
for forbidden in ["_atlas_corrections","Flavor_","Pileup_","NNJvtEfficiency","np.sqrt","np.cos","np.sin"]:
 assert forbidden not in adapter,forbidden
payload=json.loads((d/"payloads/graphs.json").read_text());ops=Counter()
def visit(steps):
 for step in steps:
  ops[step["op"]]+=1
  if step["op"]=="if":visit(step["then"]);visit(step["else"])
for program in payload["graphs"].values():visit(program["steps"])
assert sum(x.startswith("pipeline_") for x in payload["graphs"])==119
assert ops["formula"] and ops["call"] and ops["graph"] and ops["cast"]
report=dict(passed=True,graphs=len(payload["graphs"]),complete_pipelines=119,operations=dict(ops),generic_runtime_has_no_component_formulas=True,interpretation="Loaded graph interpreter; no graph-specialized C++ code generation.")
(d/"results/architecture.json").write_text(json.dumps(report,indent=2))
print(report)
