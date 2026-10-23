"""Component timings after correctness; feature preparation is reported separately from evaluation."""
import time,resource,json,sys
from pathlib import Path
import numpy as np,awkward as ak,uproot,correctionlib
r=Path(__file__).resolve().parents[1];sys.path[:0]=[str(r/"install/python"),str(r/"python")]
from _atlas_corrections import Nominal,JetSystematics
from atlas_correctionlib.jvt import Jvt
def rss():return resource.getrusage(resource.RUSAGE_SELF).ru_maxrss*1024
timings={};begin=time.perf_counter()
s=json.loads((r/"payloads/nominal-manifest.json").read_text())["settings"]
nom=Nominal(str(r/"payloads/nominal.json.gz"),s["OffsetCorrection.DefaultMuRef"],s["OffsetCorrection.DefaultNPVRef"],s["MuScaleFactor"])
syseng=JetSystematics(str(r/"payloads/uncertainty-tables.json.gz"),str(r/"payloads/uncertainty-manifest.json"))
tables=correctionlib.CorrectionSet.from_file(str(r/"payloads/uncertainty-tables.json.gz"));jvt=Jvt(r/"payloads",tables)
timings["initialization_seconds"]=time.perf_counter()-begin
path=Path("/cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt/src/AsgExternal/Asg_Test/cmt/TestFileLiteR3MC.txt").read_text().strip()
tree=uproot.open(path)["CollectionTree"]
keys=[k for k in tree.keys() if k.startswith(("AnalysisJetsAuxDyn.","PrimaryVerticesAuxDyn.","AntiKt4TruthDressedWZJetsAuxDyn.","Kt4EMPFlowNeutEventShapeAuxDyn.","EventInfoAuxDyn."))]
# Read only columns used by the implementation, avoiding unrelated decorations.
fields={"pt","eta","phi","m","JetConstitScaleMomentum_pt","JetConstitScaleMomentum_eta","JetConstitScaleMomentum_phi","JetConstitScaleMomentum_m","DetectorEta","ActiveArea4vec_pt","SumPtChargedPFOPt500","EnergyPerSampling","NumTrkPt1000","TrackWidthPt1000","SumPtTrkPt500","JVFCorr","PartonTruthLabelID","vertexType","trackParticleLinks","Density","averageInteractionsPerCrossing","eventNumber"}
keys=[k for k in keys if k.rsplit(".",1)[-1] in fields]
begin=time.perf_counter();arrays=tree.arrays(keys,entry_stop=1000);timings["direct_input_read_seconds"]=time.perf_counter()-begin
begin=time.perf_counter();d=np.load(r/"results/jvt-inputs.npz");count=d["counts"][:1000];n=int(count.sum())
inputs=np.load(r/"results/columnar-inputs.npz")["rows"][:n].tolist()
p4=d["p4"][:n];rows=np.column_stack([p4,d["mu"][:n],d["npv"][:n],d["bjet"][:n]]).tolist()
timings["cached_feature_load_seconds"]=time.perf_counter()-begin
begin=time.perf_counter();nom.batch(inputs);timings["nominal_evaluation_seconds"]=time.perf_counter()-begin
kinematics=0.;jvttime=0.
for name in syseng.names():
 begin=time.perf_counter();out=np.asarray(syseng.batch(name,rows));kinematics+=time.perf_counter()-begin
 begin=time.perf_counter();jvt.evaluate(out[:,:4],d["sumpt"][:n],d["jvfcorr"][:n],d["haspv"][:n],d["ishs"][:n]);jvttime+=time.perf_counter()-begin
timings["all_117_kinematic_states_seconds"]=kinematics;timings["nnjvt_and_weights_for_117_states_seconds"]=jvttime
begin=time.perf_counter()
for sigma in (-1,1):jvt.evaluate(p4,d["sumpt"][:n],d["jvfcorr"][:n],d["haspv"][:n],d["ishs"][:n],sigma)
timings["two_weight_only_variations_seconds"]=time.perf_counter()-begin
report=dict(scope="component-benchmark-not-end-to-end-throughput",events=1000,jets=n,input_cache="warm CVMFS/OS caches possible",feature_preparation="Features computed from original columns by validated runners; this benchmark loads their cache and does not time reconstruction.",timings=timings,peak_resident_bytes=rss())
(r/"results/performance.json").write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
