import sys,json
from pathlib import Path
import uproot
r=Path(__file__).resolve().parents[1]
m=json.loads((r/"payloads/uncertainty-manifest.json").read_text())
for k,c in m["components"].items():
 print(k,{a:b for a,b in c.items() if a!="Desc"})
print("ANALYSIS HISTOGRAMS",[k for k in m["mapping"] if k.startswith("file1") and "__Full__" in k])
path=Path("/cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt/src/AsgExternal/Asg_Test/cmt/TestFileLiteR3MC.txt").read_text().strip()
t=uproot.open(path)["CollectionTree"]
for k in t.keys():
 if "PrimaryVertices" in k or "PartonTruth" in k:print(k)
print(t.arrays(["PrimaryVerticesAuxDyn.trackParticleLinks"],entry_stop=1))
