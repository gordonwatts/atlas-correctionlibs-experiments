"""Compile the frozen ATLAS recipe into generic CorrectionLib graph programs."""
import json,gzip,hashlib,shutil,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];D=ROOT/"path-d"
def f32(x):return struct.unpack("f",struct.pack("f",float(x)))[0]
class Graph:
 def __init__(self,inputs):self.inputs=inputs;self.steps=[];self.reg=[];self.constants={}
 def emit(self,op,ins=(),n=1,**kw):
  outs=[f"r{len(self.reg)+i}" for i in range(n)];self.reg+=outs
  self.steps.append(dict(op=op,inputs=list(ins),outputs=outs,**kw))
  return outs[0] if n==1 else outs
 def c(self,x):
  x=float(x)
  if x not in self.constants:self.constants[x]=self.emit("constant",value=x)
  return self.constants[x]
 def f(self,s,*args):return self.emit("formula",args,expression=s)
 def cast(self,a,mode="float32"):return self.emit("cast",[a],mode=mode)
 def call(self,name,*args,compound=False):return self.emit("call",args,correction=name,compound=compound)
 def assign(self,target,source):self.steps.append(dict(op="formula",inputs=[source],outputs=[target],expression="x"))
 def branch(self,cond,yes,no=lambda:None):
  parent=self.steps;self.steps=[];yes();a=self.steps;self.steps=[];no();b=self.steps;self.steps=parent
  self.steps.append(dict(op="if",inputs=[cond],outputs=[],then=a,**{"else":b}))
 def finish(self,outputs):
  # Explicit local initialization makes branch-local scratch values unambiguous.
  values={v:k for k,v in self.constants.items()}
  pre=[dict(op="constant",inputs=[],outputs=[n],value=values.get(n,0.)) for n in self.reg]
  return dict(inputs=self.inputs,steps=pre+self.steps,outputs=outputs)
graphs={}
nom=json.loads((ROOT/"payloads/nominal-manifest.json").read_text())
unc=json.loads((ROOT/"payloads/uncertainty-manifest.json").read_text())
settings=nom["settings"]
g=Graph(["pt","eta","phi","mass","deteta","area","rho","mu","npv","charged","tile","em3","ntrk","width"])
positive=g.f("x>0","pt");g.emit("assert",[positive],n=0,message="nonpositive constituent pt")
ae=g.f("abs(x)","eta")
a=g.call("residual_mu",ae);b=g.call("residual_npv",ae)
dm=g.f("x*y-z","mu",g.c(settings["MuScaleFactor"]),g.c(settings["OffsetCorrection.DefaultMuRef"]))
dn=g.f("x-y","npv",g.c(settings["OffsetCorrection.DefaultNPVRef"]))
off=g.f("1000*(x*y+z*t)",a,dm,b,dn)
sf=g.call("pileup_scale","pt","area","rho",off)
pt=g.cast(g.f("x*y","pt",sf));m=g.cast(g.f("x*y","mass",sf));eta=g.cast("eta");phi=g.cast("phi")
outputs=[pt,eta,phi,m]
energy,momentum=g.emit("coordinates",[pt,eta,phi,m],n=2,mode="ptetaphim_properties")
sf=g.call("jes_scale",g.f("x/1000",energy),"deteta",g.c(0))
qp=g.f("x*y",pt,sf);qm=g.f("x*y",m,sf)
energy,momentum=g.emit("coordinates",[qp,eta,phi,qm],n=2,mode="ptetaphim_properties")
ne=g.f("x+y",eta,g.call("eta_shift",g.f("x/1000",energy),"deteta"))
np=g.f("x/cosh(y)",momentum,ne)
cart=g.emit("coordinates",[np,ne,phi,qm],n=4,mode="ptetaphim_to_cartesian")
p4=g.emit("coordinates",cart,n=4,mode="cartesian_to_ptetaphim")
pt,eta,phi,m=[g.cast(v) for v in p4];outputs += [pt,eta,phi,m]
sf=g.call("gsc",g.f("x/1000",pt),g.f("abs(x)","deteta"),g.c(0),"charged","tile","em3","ntrk","width",compound=True)
outputs += [g.cast(g.f("x*y",pt,sf)),eta,phi,g.cast(g.f("x*y",m,sf))]
graphs["nominal"]=g.finish(outputs)
recipes={}
for c in unc["components"].values():
 if "Group" not in c:recipes[c["Name"]]=[c]
for key,group in unc["groups"].items():
 assert group["Corr"]=="Correlated"
 recipes[group["Name"]]=[c for c in unc["components"].values() if c.get("Group")==key]
def table(g,name,pt,eta,mode="Full",file=0):
 name=f"file{file}__{name}_AntiKt4EMPFlow__{mode}__content"
 return g.call(name,*([pt] if table_arity[name]==1 else [pt,eta]))
def component(g,c,pt,eta):
 name=c["Name"];hist=c.get("Hists",name);mode=c.get("Interp","Full")
 y=g.f("abs(x)",eta) if c["Param"]=="PtAbsEta" else eta
 h=lambda key:table(g,key,pt,y,mode)
 if c.get("Special")!="True":return h(hist)
 dm=g.f("x-y","mu",g.c(unc["settings"]["Pileup.MuRef"]));dn=g.f("x-y","npv",g.c(unc["settings"]["Pileup.NPVRef"]))
 g.branch(g.f("x<=0","npv"),lambda:(g.assign(dm,g.c(-1234)),g.assign(dn,g.c(-1234))))
 if name=="Pileup_OffsetMu_orig":return g.f("x*y",dm,h(hist))
 if name=="Pileup_OffsetNPV_orig":return g.f("x*y",dn,h(hist))
 if name=="Pileup_PtTerm_orig":
  a,b=hist.split(",");return g.f("x*y+z*t",dm,h(a),dn,h(b))
 if name=="Pileup_RhoTopology_orig":
  weight=g.emit("constant",value=1.)
  g.branch(g.f("x<=0","npv"),lambda:g.assign(weight,g.c(-1234)))
  return g.f("x*y",weight,h(hist))
 result=g.emit("constant",value=0.)
 def compute():
  if name=="BJES_Response":v=h(hist)
  else:
   fg=table(g,"gluonFraction",pt,g.f("abs(x)",eta),file=1)
   if name=="Flavor_Response_orig":v=g.f("x*y",h(hist),fg)
   elif name=="Flavor_Composition_orig":
    rg,rq=[h(x) for x in hist.split(",")]
    df=table(g,"gluonFractionError",pt,g.f("abs(x)",eta),file=1)
    den=g.f("x*y+(1-x)*z",fg,rg,rq)
    v=g.f("x*abs(y-z)/t",df,rq,rg,den)
   else:raise ValueError(name)
  g.assign(result,v)
 cond=g.f("x!=0" if name=="BJES_Response" else "x==0","bjet")
 g.branch(cond,compute)
 return result
with gzip.open(ROOT/"payloads/nominal.json.gz","rt") as f:tables=json.load(f)
with gzip.open(ROOT/"payloads/uncertainty-tables.json.gz","rt") as f:ut=json.load(f)
tables["corrections"]+=ut["corrections"]
table_arity={c["name"]:len(c["inputs"]) for c in tables["corrections"]}
sysinputs=["pt","eta","phi","mass","mu","npv","bjet"]
g=Graph(sysinputs);graphs["NOSYS"]=g.finish(["pt","eta","phi","mass",g.c(0)])
for name,cs in sorted(recipes.items()):
 jer=cs[0].get("Scale")=="FourVecResAbs"
 for data in range(2 if jer else 1):
  for sign,label in [(1,"up"),(-1,"down")]:
   g=Graph(sysinputs);pt=g.f("x*y","pt",g.c(f32(.001)));eta="eta"
   u=g.c(0)
   for c in cs:u=g.f("x+y",u,component(g,c,pt,eta))
   variation=g.f("x*y",g.c(sign),u)
   scale=g.f("1+x",variation)
   if jer:
    ae=g.f("abs(x)",eta)
    sigma=g.f("max(x,y)",table(g,"JER_Nominal_MC16",pt,ae,"OnlyX"),table(g,"JER_Nominal_data",pt,ae,"OnlyX"))
    scale=g.emit("constant",value=1.)
    def draw():
     width=g.f("sqrt(pow(x+abs(y),2)-pow(x,2))",sigma,variation)
     seed=g.cast(g.f("x*abs(y)",g.c(1.23e5 if data else 2e5),"phi"),"int64")
     g.branch(g.f("x==0",seed),lambda:g.assign(seed,g.c(34545654 if data else 45583453)))
     factor=g.emit("gaussian",[seed,g.c(1),width,g.c(0)],algorithm="ROOT-TRandom3-Gaus")
     g.assign(scale,factor)
    g.branch(g.f("x<0" if data else "x>0",variation),draw)
   normalized=g.emit("coordinates",[g.f("x*y","pt",scale),"eta","phi",g.f("x*y","mass",scale)],n=4,mode="ptetaphim_normalize")
   out=[g.cast(x) for x in normalized]+[g.c(0)]
   graphs["JET_"+name+("_PseudoData" if data else "")+"__1"+label]=g.finish(out)
# Explicit NNJVT feature/selection graph, with generic lwtnn inference boundary.
cuts=json.loads((ROOT/"payloads/nnjvt-cuts.json").read_text())
pe=[f32(f32(x)*f32(1000)) for x in cuts["ptbin_edges"]];ee=[f32(x) for x in cuts["etabin_edges"]]
g=Graph(["pt","eta","sumpt","jvfcorr","haspv"])
pt=g.cast("pt");eta=g.cast("eta")
def lower_bound(x,edges):
 index=g.c(len(edges))
 for i,edge in reversed(list(enumerate(edges))):
  out=g.emit("formula",[index],expression="x")
  g.branch(g.f("x<=y",x,g.c(edge)),lambda out=out,i=i:g.assign(out,g.c(i)))
  index=out
 return index
ip=lower_bound(pt,pe);ie=lower_bound(eta,ee)
g.branch(g.f("x==0",ip),lambda:g.assign(ip,g.c(1)))
ok=g.f("(x<y)*(z>0)*(z<t)",ip,g.c(len(pe)),ie,g.c(len(ee)))
ib=g.f("x-1",ip);eb=g.f("x-1",ie)
score=g.emit("constant",value=-1.);cut=g.emit("constant",value=-1.)
def infer():
 rpt=g.emit("constant",value=0.)
 g.branch(g.f("x!=0","pt"),lambda:g.assign(rpt,g.cast(g.f("x/y","sumpt","pt"))))
 n=g.emit("inference",[rpt,g.cast("jvfcorr"),g.cast(ib),g.cast(eb)],model="nnjvt-network.json",features=["Rpt","JVFCorr","ptbin","etabin"],input_node=json.loads((ROOT/"payloads/nnjvt-network.json").read_text())["inputs"][0]["name"],output_index=0)
 g.assign(score,g.cast(n))
g.branch(g.f("(x!=0)*(y<=60000)*(z!=0)","haspv","pt",ok),infer)
for i in range(len(pe)-1):
 for j in range(len(ee)-1):
  g.branch(g.f("(x==y)*(z==t)",ib,g.c(i),eb,g.c(j)),lambda i=i,j=j:g.assign(cut,g.c(f32(cuts["cuts"][f"({i}, {j})"]))))
valid=g.f("(x>=20000)*(x<=60000)*(abs(y)<=2.5)","pt",eta)
passed=g.f("max(x==0,y>z)",valid,score,cut)
graphs["nnjvt"]=g.finish([score,passed])
# SF graph: each float operation rounded at the same source sequence point.
bounds={}
for key,v in unc["inventory"].items():
 for h in ["JvtFixedEffPt","EffFixedEffPt"]:
  if key.endswith("::"+h):bounds[h]=v["bounds"]
for shift in [-1,0,1]:
 g=Graph(["pt","eta","pass","truthHS"])
 pt=g.cast("pt");eta=g.cast("eta")
 valid=g.f("(x>=20000)*(x<=60000)*(abs(y)<=2.5)","pt",eta)
 weight=g.emit("constant",value=-1.);status=g.emit("constant",value=1.)
 def sf():
  def truth():
   sfvalid=g.c(1)
   for h in ["JvtFixedEffPt","EffFixedEffPt"]:
    lo,hi,el,eh=bounds[h]
    cond=g.f("(x>=y)*(x<z)",pt,g.c(lo),g.c(hi))
    cond=g.f("x*(y>=z)*(y<t)",cond,eta,g.c(el),g.c(eh))
    if h=="EffFixedEffPt":cond=g.f("max(x!=0,y)","pass",cond)
    sfvalid=g.f("x*y",sfvalid,cond)
   def calc():
    def shifted(h):
     base=g.cast(g.call(f"file2__{h}__None__content",pt,eta))
     err=g.cast(g.call(f"file2__{h}__None__error",pt,eta))
     return g.cast(g.f("x+y",base,g.cast(g.f("x*y",err,g.c(shift)))))
    base=shifted("JvtFixedEffPt");g.assign(weight,base)
    def failing():
     eff=shifted("EffFixedEffPt")
     numerator=g.cast(g.f("1-x",g.cast(g.f("x*y",base,eff))))
     denominator=g.cast(g.f("1-x",eff))
     g.assign(weight,g.cast(g.f("x/y",numerator,denominator)))
    g.branch(g.f("x==0","pass"),failing)
    g.assign(status,g.c(0))
   g.branch(sfvalid,calc)
  def nontruth():g.assign(weight,g.c(1));g.assign(status,g.c(0))
  g.branch(g.f("x!=0","truthHS"),truth,nontruth)
 g.branch(valid,sf)
 graphs["efficiency_"+str(shift)]=g.finish([weight,status])

# Raw nominal features are part of the payload, not handwritten adapter physics.
prepared=graphs["nominal"];graphs["nominal_prepared"]=prepared
g=Graph(["pt","eta","phi","mass","deteta","area","rho","mu","npv","charged_sum","sample12","sample18","sample3","sample7","ntrk","width"])
energy=g.cast(g.f("sqrt(pow(x*cosh(y),2)+z*z)","pt","eta","mass"))
charged=g.f("x/y","charged_sum","pt")
tile=g.cast(g.f("x/y",g.cast(g.f("x+y","sample12","sample18")),energy))
em3=g.cast(g.f("x/y",g.cast(g.f("x+y","sample3","sample7")),energy))
mapping={s:s for s in prepared["inputs"]};mapping.update(charged=charged,tile=tile,em3=em3)
def renamed(block):
 result=[]
 for step in block:
  step=dict(step)
  for key in ["inputs","outputs"]:
   step[key]=[mapping.get(x,"nom_"+x) for x in step.get(key,[])]
  if step["op"]=="if":
   step["then"]=renamed(step["then"]);step["else"]=renamed(step["else"])
  result.append(step)
 return result
raw=g.finish([]);raw["steps"]+=renamed(prepared["steps"]);raw["outputs"]=[mapping.get(x,"nom_"+x) for x in prepared["outputs"]]
graphs["nominal"]=raw
# Pairwise truth association is evaluated numerically by a graph; the adapter
# enumerates candidate pairs and applies an ordinary any-reduction by jet.
g=Graph(["eta","phi","truthpt","trutheta","truthphi"])
dp=g.f("x-y","phi","truthphi")
g.branch(g.f("x>y",dp,g.c(3.141592653589793)),lambda:g.assign(dp,g.f("x-y",dp,g.c(6.283185307179586))))
g.branch(g.f("x<=y",dp,g.c(-3.141592653589793)),lambda:g.assign(dp,g.f("x+y",dp,g.c(6.283185307179586))))
dr=g.cast(g.f("sqrt(pow(x-y,2)+z*z)","eta","trutheta",dp))
match=g.f("(x<y)*(z>10000)",dr,g.c(f32(.3)),"truthpt")
graphs["truth_match"]=g.finish([match])
# Selected source components fall back to true when no validity histogram is configured.
assert not any("valid" in k.lower() and str(v).strip() for k,v in unc["settings"].items()),"Validity histogram needs explicit conversion"
assert not any("valid" in k.lower() and str(v).strip() for c in unc["components"].values() for k,v in c.items()),"Component validity histogram needs explicit conversion"


# Multi-output graph calls declare selection -> weight order in the payload.
for sigma in [-1,0,1]:
 g=Graph(["pt","eta","sumpt","jvfcorr","haspv","truthHS"])
 score,passed=g.emit("graph",["pt","eta","sumpt","jvfcorr","haspv"],n=2,graph="nnjvt")
 sf,status=g.emit("graph",["pt","eta",passed,"truthHS"],n=2,graph="efficiency_"+str(sigma))
 graphs["jvt_"+str(sigma)]=g.finish([score,passed,sf,status])
# Complete scalar-jet pipeline is also expressible with shared graph calls.
# The diagnostic runners call individual stages to save first-divergence evidence.
for state in [n for n in graphs if n=="NOSYS" or n.startswith("JET_")]+["JET_NNJvtEfficiency__1up","JET_NNJvtEfficiency__1down"]:
 sigma=(1 if state.endswith("up") else -1) if "NNJvtEfficiency" in state else 0
 kinstate="NOSYS" if sigma else state
 inputs=graphs["nominal"]["inputs"]+["uncertainty_npv","bjet","sumpt","jvfcorr","haspv","truthHS"]
 g=Graph(inputs)
 stages=g.emit("graph",graphs["nominal"]["inputs"],n=12,graph="nominal")
 p4status=g.emit("graph",stages[8:12]+["mu","uncertainty_npv","bjet"],n=5,graph=kinstate)
 weights=g.emit("graph",p4status[:2]+["sumpt","jvfcorr","haspv","truthHS"],n=4,graph="jvt_"+str(sigma))
 graphs["pipeline_"+state]=g.finish(stages+p4status+weights)

bundle=dict(graph_schema_version=1,requires="correctionlib-2.7.0+graph-1",correction_set_file="corrections.json.gz",graphs=graphs)
source_hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [ROOT/"payloads/nominal.json.gz",ROOT/"payloads/uncertainty-tables.json.gz"]}
source_record=D/"payloads/table-sources.json"
if not (D/"payloads/corrections.json.gz").exists() or not source_record.exists() or json.loads(source_record.read_text())!=source_hashes:
 with gzip.open(D/"payloads/corrections.json.gz","wt",compresslevel=1) as f:json.dump(tables,f,separators=(",",":"),allow_nan=False)
source_record.write_text(json.dumps(source_hashes,sort_keys=True))
from schema import GraphBundle
GraphBundle.model_validate(bundle)
(D/"schemas/graph-v1.json").write_text(json.dumps(GraphBundle.model_json_schema(by_alias=True),indent=2))
(D/"payloads/graphs.json").write_text(json.dumps(bundle,separators=(",",":"),allow_nan=False))
shutil.copy2(ROOT/"payloads/nnjvt-network.json",D/"payloads/nnjvt-network.json")
manifest=dict(graph_schema_version=1,graphs=list(graphs),source_manifests={},runtime_boundaries=["ROOT coordinate conversions","ROOT TRandom3::Gaus","lwtnn FastGraph<double>"],base_commit="fd69c2ef33edbe9a1d4bc714430d6ccda374be00")
for path in [ROOT/"payloads/nominal-manifest.json",ROOT/"payloads/uncertainty-manifest.json",ROOT/"patches/correctionlib-full-precision-json.patch"]:
 manifest["source_manifests"][str(path.relative_to(ROOT))]=hashlib.sha256(path.read_bytes()).hexdigest()
kinematic_states=sorted(n for n in graphs if n=="NOSYS" or n.startswith("JET_"))
manifest["kinematic_states"]=kinematic_states
manifest["states"]=[dict(name=n,kinematic_graph=n,sigma=0,jvt_graph="jvt_0") for n in kinematic_states]+[dict(name="JET_NNJvtEfficiency__1up",kinematic_graph="NOSYS",sigma=1,jvt_graph="jvt_1"),dict(name="JET_NNJvtEfficiency__1down",kinematic_graph="NOSYS",sigma=-1,jvt_graph="jvt_-1")]
manifest["graph_sha256"]=hashlib.sha256((D/"payloads/graphs.json").read_bytes()).hexdigest()
(D/"payloads/manifest.json").write_text(json.dumps(manifest,indent=2))
print("Compiled",len(graphs),"graphs")
