from pathlib import Path
import uproot
r=Path(__file__).resolve().parents[1]
p=Path("/cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt/src/AsgExternal/Asg_Test/cmt/TestFileLiteR3MC.txt").read_text().strip()
t=uproot.open(p)["CollectionTree"]
print([k for k in t.keys() if ("AnalysisJets" in k and ("JVF" in k or "SumPtTrk" in k)) or ("AntiKt4TruthDressedWZJets" in k and (k.endswith((".pt",".eta",".phi")) or "Aux." in k))])
print(t["AntiKt4TruthDressedWZJetsAux."].keys())
ref=uproot.open(r/"results/reference/reference.root")["analysis"]
print([k for k in ref.keys() if "NNJvt" in k or "NNJVT" in k])
