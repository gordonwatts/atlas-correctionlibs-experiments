"""Direct PHYSLITE input -> C++ evaluator; reference used only for comparison."""
import argparse,json,sys,math
from pathlib import Path
import numpy as np,awkward as ak,uproot
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/"install/python"))
import correctionlib
from _atlas_corrections import Nominal
ap=argparse.ArgumentParser()
ap.add_argument("--events",type=int,default=100)
ap.add_argument("--chunk-size",type=int,default=100)
args=ap.parse_args()
from atlas_jet_corrections import JetCorrections, EVENT_BRANCHES
engine=JetCorrections.load(root/"payloads")
testfile=Path("/cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt/src/AsgExternal/Asg_Test/cmt/TestFileLiteR3MC.txt").read_text().strip()
fields=["JetConstitScaleMomentum_"+v for v in ["pt","eta","phi","m"]]+["DetectorEta","ActiveArea4vec_pt","SumPtChargedPFOPt500","EnergyPerSampling","NumTrkPt1000","TrackWidthPt1000"]
jprefix="AnalysisJetsAuxDyn."
branches=[jprefix+x for x in fields]+["PrimaryVerticesAuxDyn.vertexType","Kt4EMPFlowNeutEventShapeAuxDyn.Density","EventInfoAuxDyn.averageInteractionsPerCrossing","EventInfoAuxDyn.eventNumber"]
tree=uproot.open(testfile)["CollectionTree"]
outputs=[];counts=[];events=[];allrows=[]
for begin in range(0,min(args.events,tree.num_entries),args.chunk_size):
    arr=tree.arrays(branches,entry_start=begin,entry_stop=min(begin+args.chunk_size,args.events),library="ak",how=dict)
    result = engine.evaluate({f: arr[jprefix+f] for f in fields}, {name: arr[branch] for name,branch in EVENT_BRANCHES.items()})
    counts.extend(result["counts"])
    events.extend(ak.to_list(arr["EventInfoAuxDyn.eventNumber"]))
    allrows.extend(result["inputs"])
    outputs.extend(result["stages"])
    print("Processed",begin+len(arr[branches[0]]),"events",flush=True)
np.savez(root/"results/columnar-inputs.npz",rows=np.asarray(allrows,dtype=np.float64))
out=np.asarray(outputs,dtype=np.float64)
np.savez(root/"results/columnar-nominal.npz",values=out,counts=counts,eventNumber=events)
reference=uproot.open(root/"results/reference/reference.root")["analysis"]
checks={}
for stage,base in enumerate(["JetPileupScaleMomentum","JetEtaJESScaleMomentum","JetGSCScaleMomentum"]):
    for k,coord in enumerate(["pt","eta","phi","m"]):
        r=ak.to_numpy(ak.flatten(reference[base+"_"+coord].array(entry_stop=args.events)))
        v=out[:,4*stage+k]
        assert len(v)==len(r)
        delta=np.abs(v-r)
        if coord=="phi":delta=np.abs((v-r+np.pi)%(2*np.pi)-np.pi)
        tol=2e-6 if coord in ("eta","phi") else 0.01+2e-6*np.abs(r)
        bad=(delta>tol)|~np.isfinite(v)|~np.isfinite(r)
        checks[base+"_"+coord]=dict(max_abs=float(np.max(delta,initial=0)),mismatches=int(np.sum(bad)),first_bad=np.flatnonzero(bad)[:10].tolist())
report=dict(events=len(counts),jets=len(out),chunk_size=args.chunk_size,scope="nominal-only",checks=checks,passed=all(x["mismatches"]==0 for x in checks.values()))
(root/"results/nominal-comparison.json").write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
sys.exit(0 if report["passed"] else 1)
