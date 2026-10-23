"""Load original ATLAS constants with ROOT TEnv and PathResolver; track provenance."""
from pathlib import Path
import hashlib
import ROOT
from PathResolver import PathResolver

class Sources:
    def __init__(self): self.files={}
    def resolve(self,path):
        resolved=PathResolver.FindCalibFile(str(path))
        if not resolved: raise FileNotFoundError(path)
        p=Path(resolved)
        if str(p) not in self.files:
            self.files[str(p)]=dict(logical_path=str(path),sha256=hashlib.sha256(p.read_bytes()).hexdigest(),size=p.stat().st_size)
        return str(p)
    def env(self,path):
        env=ROOT.TEnv()
        if env.ReadFile(self.resolve(path),ROOT.kEnvLocal): raise ValueError(path)
        return env
    def merge(self,env,path):
        if env.ReadFile(self.resolve(path),ROOT.kEnvLocal): raise ValueError(path)
    def root(self,path):
        f=ROOT.TFile.Open(self.resolve(path))
        if not f or f.IsZombie(): raise ValueError(path)
        return f

def vector(env,key):
    s=env.GetValue(key,"")
    if not s: raise KeyError(key)
    return [float(v) for v in s.split()]

def centres(axis):
    return [axis.GetBinCenter(i) for i in range(1,axis.GetNbins()+1)]

def edges(axis):
    return [axis.GetBinLowEdge(i) for i in range(1,axis.GetNbins()+2)]
