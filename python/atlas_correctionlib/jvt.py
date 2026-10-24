"""NNJVT and efficiency calculation using bundled model and converted tables."""
import json,gzip
from pathlib import Path
import numpy as np
from _atlas_corrections import NNJvt,JvtEfficiency
from .parallel import batch as parallel_batch
class Jvt:
 def __init__(self,payload,tables):
  payload=Path(payload);c=json.loads((payload/"nnjvt-cuts.json").read_text())
  pt=(np.asarray(c["ptbin_edges"],dtype=np.float32)*np.float32(1000)).tolist()
  eta=np.asarray(c["etabin_edges"],dtype=np.float32).tolist()
  cuts=[[c["cuts"][f"({i}, {j})"] for j in range(len(eta)-1)] for i in range(len(pt)-1)]
  self.nn=NNJvt(str(payload/"nnjvt-network.json"),pt,eta,cuts);self.tables=tables
  self.efficiency=JvtEfficiency(str(payload/"uncertainty-tables.json.gz"),str(payload/"uncertainty-manifest.json"))
  self._payload=payload
  self._ranges=None
  self._anchor=None
 @property
 def ranges(self):
  if self._ranges is None:
    with gzip.open(self._payload/"uncertainty-tables.json.gz","rt") as f:data=json.load(f)
    self._ranges={}
    for h in ["JvtFixedEffPt","EffFixedEffPt"]:
     node=next(x for x in data["corrections"] if x["name"]==f"file2__{h}__None__content")["data"]
     self._ranges[h]=(node["edges"][0],node["edges"][-1],node["content"][0]["edges"][0],node["content"][0]["edges"][-1])
  return self._ranges
 def evaluate(self,p4,sumpt,jvfcorr,haspv,ishs,sigma=0):
  # Keep one exact sigma-zero result. Copies protect against caller mutation.
  inputs=(p4[:,:2],sumpt,jvfcorr,haspv,ishs)
  if sigma==0 and self._anchor is not None:
   nn,efficiency,saved,result=self._anchor
   if nn is self.nn and efficiency is self.efficiency and all(np.array_equal(a,b) for a,b in zip(inputs,saved)):
    return tuple(value.copy() for value in result)
  # Both NNJVT selection and efficiency use the configured default JetEtaName='eta'.
  rows=np.column_stack([p4[:,0],p4[:,1],p4[:,1],sumpt,jvfcorr,haspv])
  result=np.asarray(parallel_batch(self.nn.batch_array,rows));score=result[:,0];passed=result[:,1].astype(bool)
  values=np.asarray(parallel_batch(lambda chunk:self.efficiency.batch_array(chunk,sigma),np.column_stack([p4[:,0],p4[:,1],passed,ishs])))
  output=(score,passed,values[:,0].astype(np.float32),values[:,1]==0)
  if sigma==0 and self._anchor is None:
   self._anchor=(self.nn,self.efficiency,tuple(np.array(value,copy=True) for value in inputs),tuple(value.copy() for value in output))
  return output
