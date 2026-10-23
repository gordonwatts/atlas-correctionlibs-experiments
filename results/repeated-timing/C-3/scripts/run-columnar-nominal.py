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
manifest=json.loads((root/"payloads/nominal-manifest.json").read_text())
settings=manifest["settings"]
engine=Nominal(str(root/"payloads/nominal.json.gz"),settings["OffsetCorrection.DefaultMuRef"],settings["OffsetCorrection.DefaultNPVRef"],settings["MuScaleFactor"])
testfile=Path("/cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt/src/AsgExternal/Asg_Test/cmt/TestFileLiteR3MC.txt").read_text().strip()
fields=["JetConstitScaleMomentum_"+v for v in ["pt","eta","phi","m"]]+["DetectorEta","ActiveArea4vec_pt","SumPtChargedPFOPt500","EnergyPerSampling","NumTrkPt1000","TrackWidthPt1000"]
jprefix="AnalysisJetsAuxDyn."
branches=[jprefix+x for x in fields]+["PrimaryVerticesAuxDyn.vertexType","Kt4EMPFlowNeutEventShapeAuxDyn.Density","EventInfoAuxDyn.averageInteractionsPerCrossing","EventInfoAuxDyn.eventNumber"]
tree=uproot.open(testfile)["CollectionTree"]
outputs=[];counts=[];events=[];allrows=[]
for begin in range(0,min(args.events,tree.num_entries),args.chunk_size):
    arr=tree.arrays(branches,entry_start=begin,entry_stop=min(begin+args.chunk_size,args.events),library="ak",how=dict)
    rows=[]
    for i in range(len(arr[branches[0]])):
        types=ak.to_list(arr["PrimaryVerticesAuxDyn.vertexType"][i])
        pv=next((k for k,t in enumerate(types) if t==1),0)
        npv=sum(t in (1,3) for t in types)
        rho=float(arr["Kt4EMPFlowNeutEventShapeAuxDyn.Density"][i])
        mu=float(arr["EventInfoAuxDyn.averageInteractionsPerCrossing"][i])
        jets={f:ak.to_list(arr[jprefix+f][i]) for f in fields}
        n=len(jets["DetectorEta"]);counts.append(n);events.append(int(arr["EventInfoAuxDyn.eventNumber"][i]))
        for j in range(n):
            pt,eta,phi,m=(float(jets["JetConstitScaleMomentum_"+x][j]) for x in ["pt","eta","phi","m"])
            energy=np.float32(math.sqrt((pt*math.cosh(eta))**2+m*m))
            sample=np.asarray(jets["EnergyPerSampling"][j],dtype=np.float32)
            tile=float(np.float32((sample[12]+sample[18])/energy))
            em3=float(np.float32((sample[3]+sample[7])/energy))
            charged=float(jets["SumPtChargedPFOPt500"][j][pv])/pt
            rows.append([pt,eta,phi,m,float(jets["DetectorEta"][j]),float(jets["ActiveArea4vec_pt"][j]),rho,mu,float(npv),charged,tile,em3,float(jets["NumTrkPt1000"][j][pv]),float(jets["TrackWidthPt1000"][j][pv])])
    allrows.extend(rows)
    outputs.extend(engine.batch(rows))
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
