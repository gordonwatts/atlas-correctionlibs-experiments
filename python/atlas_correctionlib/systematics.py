"""Prototype vectorized JES/JER orchestration over converted CorrectionLib tables.
No xAOD objects or original calibration ROOT payloads are used.
"""
import json
from pathlib import Path
import numpy as np, correctionlib
from _atlas_corrections import smear_batch
from .parallel import batch as parallel_batch
class PythonRecipePrototype:
 def __init__(self,payload):
  payload=Path(payload)
  self.tables=correctionlib.CorrectionSet.from_file(str(payload/"uncertainty-tables.json.gz"))
  self.manifest=json.loads((payload/"uncertainty-manifest.json").read_text())
  self.components=self.manifest["components"]
  self.groups=self.manifest["groups"]
  self.recipes={}
  for cid,c in self.components.items():
   if "Group" not in c:self.recipes[c["Name"]]=[c]
  for gid,g in self.groups.items():
   if g.get("Corr")!="Correlated":raise ValueError("Unsupported group correlation")
   self.recipes[g["Name"]]=[c for c in self.components.values() if c.get("Group")==gid]
 def table(self,name,pt,eta,mode="Full",file=0):
  c=self.tables[f"file{file}__{name}_AntiKt4EMPFlow__{mode}__content"]
  return c.evaluate(pt,*([eta] if len(c.inputs)>1 else []))
 def component(self,c,pt,eta,mu,npv,bjet):
  name=c["Name"];mode=c.get("Interp","Full")
  y=np.abs(eta) if c["Param"]=="PtAbsEta" else eta
  h=lambda n:self.table(n,pt,y,mode)
  hist=c.get("Hists",name)
  if c.get("Special")!="True":return h(hist)
  settings=self.manifest["settings"]
  if name.startswith("Pileup_"):
   dm=mu-float(settings["Pileup.MuRef"]);dn=npv-float(settings["Pileup.NPVRef"])
   if name=="Pileup_OffsetMu_orig":return dm*h(hist)
   if name=="Pileup_OffsetNPV_orig":return dn*h(hist)
   if name=="Pileup_PtTerm_orig":
    a,b=hist.split(",");return dm*h(a)+dn*h(b)
   if name=="Pileup_RhoTopology_orig":return h(hist)
  if name=="BJES_Response":return np.where(bjet,h(hist),0.)
  fg=self.table("gluonFraction",pt,np.abs(eta),file=1)
  if name=="Flavor_Response_orig":return np.where(bjet,0.,h(hist)*fg)
  if name=="Flavor_Composition_orig":
   rg,rq=(h(n) for n in hist.split(","))
   df=self.table("gluonFractionError",pt,np.abs(eta),file=1)
   return np.where(bjet,0.,df*np.abs(rq-rg)/(fg*rg+(1-fg)*rq))
  raise ValueError(("Unsupported component",c))
 def variations(self,p4,mu,npv,bjet):
  # The ATLAS uncertainty interfaces store the energy conversion as float.
  pt=p4[:,0]*float(np.float32(.001));eta=p4[:,1]
  yield "NOSYS",p4
  for name,recipe in self.recipes.items():
   unc=sum(self.component(c,pt,eta,mu,npv,bjet) for c in recipe)
   jer=recipe[0].get("Scale")=="FourVecResAbs"
   for data in ([False,True] if jer else [False]):
    for sign,label in [(1,"up"),(-1,"down")]:
     if jer:
      sigma=np.maximum(self.table("JER_Nominal_MC16",pt,np.abs(eta),"OnlyX"),self.table("JER_Nominal_data",pt,np.abs(eta),"OnlyX"))
      scale=np.asarray(smear_batch(p4[:,2],sigma,sign*unc,data))
     else:scale=1.+sign*unc
     out=p4.copy()
     out[:,0]=(p4[:,0]*scale).astype(np.float32)
     out[:,3]=(p4[:,3]*scale).astype(np.float32)
     yield "JET_"+name+("_PseudoData" if data else "")+"__1"+label,out


class Systematics:
 """Production entry point: all JES/JER arithmetic lives in the shared C++ layer."""
 def __init__(self,payload):
  from _atlas_corrections import JetSystematics
  payload=Path(payload)
  self.tables=correctionlib.CorrectionSet.from_file(str(payload/"uncertainty-tables.json.gz"))
  self.engine=JetSystematics(str(payload/"uncertainty-tables.json.gz"),str(payload/"uncertainty-manifest.json"))
 def variations(self,p4,mu,npv,bjet):
  rows=np.column_stack([p4,mu,npv,bjet])
  for name in self.engine.names():
   values=np.asarray(parallel_batch(lambda chunk:self.engine.batch_array(name,chunk),rows))
   if np.any(values[:,4]!=0):raise RuntimeError("Unsuccessful systematic correction status")
   yield name,values[:,:4]
