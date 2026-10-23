"""Scalar/batch, serialization, numerical and rejection tests for generic primitives."""
import sys,json,gzip,tempfile,struct,hashlib
from pathlib import Path
import numpy as np,ROOT
r=Path(__file__).resolve().parents[2];sys.path[:0]=[str(r/"path-d/python"),str(r/"path-d/build")]
from schema import GraphBundle
from _declarative import GraphSet
payload=r/"path-d/payloads/graphs.json"
schema=GraphBundle.model_json_schema(by_alias=True)
(r/"path-d/schemas/graph-v1.json").write_text(json.dumps(schema,indent=2))
model=GraphBundle.model_validate_json(payload.read_text())
roundtrip=model.model_dump_json(by_alias=True)
assert GraphBundle.model_validate_json(roundtrip)==model
checks=[]
def test(name,fn):
 fn();checks.append(dict(name=name,passed=True))
def make_graph(steps,inputs,outputs):
 return dict(inputs=inputs,steps=steps,outputs=outputs)
def op(name,ins,outs,**k):return dict(op=name,inputs=ins,outputs=outs,**k)
with tempfile.TemporaryDirectory(dir=r/"path-d/results") as tmp:
 tmp=Path(tmp)
 table=dict(schema_version=2,corrections=[dict(name="linear",version=1,inputs=[dict(name="x",type="real")],output=dict(name="y",type="real"),data=dict(nodetype="formula",expression="x*2+1",parser="TFormula",variables=["x"]))])
 with gzip.open(tmp/"table.json.gz","wt") as f:json.dump(table,f)
 graphs={
  "cast":make_graph([op("cast",["x"],["y"],mode="float32")],["x"],["y"]),
  "integer":make_graph([op("cast",["x"],["y"],mode="int64")],["x"],["y"]),
  "call":make_graph([op("call",["x"],["a"],correction="linear"),op("formula",["a","x"],["y"],expression="x+y")],["x"],["y"]),
  "branch":make_graph([op("if",["condition"],[],**{"then":[op("formula",["x"],["y"],expression="x+1")],"else":[op("formula",["x"],["y"],expression="x-1")]})],["condition","x"],["y"]),
  "random":make_graph([op("gaussian",["seed","mean","width","lower"],["x"],algorithm="ROOT-TRandom3-Gaus")],["seed","mean","width","lower"],["x"]),
  "nested":make_graph([op("graph",["x"],["y"],graph="call"),op("cast",["y"],["z"],mode="float32")],["x"],["z"]),
  "coordinate":make_graph([op("coordinates",["pt","eta","phi","mass"],["px","py","pz","e"],mode="ptetaphim_to_cartesian"),op("coordinates",["px","py","pz","e"],["p","a","f","m"],mode="cartesian_to_ptetaphim")],["pt","eta","phi","mass"],["p","a","f","m"])
 }
 bundle=dict(graph_schema_version=1,requires="correctionlib-2.7.0+graph-1",correction_set_file="table.json.gz",graphs=graphs)
 def load(b=bundle):
  GraphBundle.model_validate(b);(tmp/"graphs.json").write_text(json.dumps(b));return GraphSet(str(tmp/"graphs.json"))
 engine=load()
 def same(actual,expected):assert actual==expected,(actual,expected)
 test("float32_scalar_and_batch",lambda:same(engine.batch("cast",[[1/3],[16777217],[-1/3]]),[[float(np.float32(x))] for x in [1/3,16777217,-1/3]]))
 test("integer_truncation",lambda:same(engine.batch("integer",[[2.9],[-2.9]]),[[2.],[-2.]]))
 test("shared_call_and_formula",lambda:same(engine.evaluate("call",[4]),[13.]))
 test("named_graph_call_scalar_batch",lambda:same(engine.batch("nested",[[4.],[5.]]),[[13.],[16.]]))
 test("lazy_conditional",lambda:same(engine.batch("branch",[[0,4],[1,4]]),[[3.],[5.]]))
 def randoms():
  rows=[[seed,1.,width,0.] for seed in [1,45583453,34545654,4294967295] for width in [0.,.1,2.,100.]]
  result=engine.batch("random",rows)
  expected=[]
  for seed,mean,width,lower in rows:
   rand=ROOT.TRandom3(int(seed));v=-1.
   while v<lower:v=rand.Gaus(mean,width)
   expected.append([v])
  same(result,expected)
  same(result,[engine.evaluate("random",row) for row in rows])
 test("exact_ROOT_gaussian_rejection_scalar_batch",randoms)
 def coordinates():
  rows=[[10000.,eta,.3,m] for eta in [-4.5,0.,2.5] for m in [0.,1.,1000.]]
  same(engine.batch("coordinate",rows),[engine.evaluate("coordinate",x) for x in rows])
 test("coordinate_scalar_batch",coordinates)
 def rejection():
  for name,row in [("random",[0.,1.,1.,0.]),("random",[1.,1.,-1.,0.]),("cast",[float("nan")]),("integer",[1e30])]:
   try:engine.evaluate(name,row)
   except (ValueError,RuntimeError):pass
   else:raise AssertionError((name,row))
  for mutation in ["unknown_operation","unknown_version","undefined_register","wrong_arity","cycle"]:
   b=json.loads(json.dumps(bundle))
   if mutation=="unknown_operation":b["graphs"]["cast"]["steps"][0]["op"]="atlas_opaque"
   if mutation=="unknown_version":b["requires"]="future"
   if mutation=="undefined_register":b["graphs"]["cast"]["steps"][0]["inputs"]=["missing"]
   if mutation=="cycle":b["graphs"]["nested"]["steps"][0]["graph"]="nested"
   if mutation=="wrong_arity":b["graphs"]["cast"]["steps"][0]["inputs"]=[]
   try:load(b)
   except (ValueError,RuntimeError):pass
   else:raise AssertionError(mutation)
 test("invalid_payload_and_input_rejection",rejection)
report=dict(passed=True,checks=checks,graph_schema_version=1,payload_sha256=hashlib.sha256(payload.read_bytes()).hexdigest(),serialized_roundtrip=True)
(r/"path-d/results/primitives.json").write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
