import json,sys,math
from pathlib import Path
import numpy as np,uproot,awkward as ak,correctionlib,ROOT
r=Path(__file__).resolve().parents[1];sys.path.insert(0,str(r/"python"))
from atlas_correctionlib.payloads import Sources,vector
from atlas_correctionlib.compile_nominal import CONFIG,area_path
src=Sources();env=src.env(CONFIG);src.merge(env,area_path(env.GetValue("AbsoluteJES.CalibFile","")))
f=src.root(area_path(env.GetValue("AbsoluteJES.CalibHists","")));hs=f.Get("etaJes")
cs=correctionlib.CorrectionSet.from_file(str(r/"payloads/nominal.json.gz"))
ROOT.gInterpreter.Declare("""
#include <Math/Vector4D.h>
#include <TLorentzVector.h>
std::vector<double> diagnoseEta(std::vector<double> in,TH1* h,std::vector<double> coeff,double deteta,double cutoff){
 ROOT::Math::PtEtaPhiMVector p(in[0],in[1],in[2],in[3]);
 double ei=p.E()/1000.;
 double sf=1./h->Interpolate(ei);
 if(ei/cosh(deteta)<15){
  int b=h->FindBin(cutoff);
  double slope=(h->GetBinContent(b+1)-h->GetBinContent(b))/(h->GetBinCenter(b+1)-h->GetBinCenter(b));
  sf=1./(slope*(ei-cutoff)+h->Interpolate(cutoff));
 }
 p=p*sf;
 double e=p.E()/1000.;e=std::min(std::max(e,8*cosh(deteta)),2500.);
 double sum=0.;for(unsigned i=0;i<coeff.size();++i)sum+=coeff[i]*TMath::Power(log(e),Int_t(i));
 double eta=p.Eta()-sum;
 TLorentzVector t;t.SetPtEtaPhiM(p.P()/cosh(eta),eta,p.Phi(),p.M());
 p.SetPxPyPzE(t.Px(),t.Py(),t.Pz(),t.E());
 return {p.Pt(),p.Eta(),p.Phi(),p.M(),e,-sum,ei,sf};
}
""")
stored=np.load(r/"results/columnar-nominal.npz")["values"]
ref=uproot.open(r/"results/reference/reference.root")["analysis"]
mass=ak.to_numpy(ak.flatten(ref["JetEtaJESScaleMomentum_m"].array()))
bad=np.flatnonzero(abs(stored[:,7]-mass)>.01+2e-6*abs(mass))
path=Path("/cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt/src/AsgExternal/Asg_Test/cmt/TestFileLiteR3MC.txt").read_text().strip()
det=ak.to_numpy(ak.flatten(uproot.open(path)["CollectionTree"]["AnalysisJetsAuxDyn.DetectorEta"].array()))
print("Calculated mismatches",bad.tolist(),flush=True)
bad=json.loads((r/"results/nominal-comparison.json").read_text())["checks"]["JetEtaJESScaleMomentum_m"]["first_bad"]
for j in bad:
 p=stored[j,:4];d=float(det[j]);ib=int((d+4.5)/.1);h=hs.At(ib);e=math.sqrt((p[0]*math.cosh(p[1]))**2+p[3]**2)/1000
 sf=1./h.Interpolate(e);sfc=cs["jes_scale"].evaluate(e,d,0.)
 coeff=vector(env,"EtaCorr.AntiKt4EMPFlow_Bin"+str(ib))
 native=list(ROOT.diagnoseEta(p.tolist(),h,coeff,d,15*math.cosh(.1*ib-4.5)))
 etac=cs["eta_shift"].evaluate(native[4],d)
 sfc=cs["jes_scale"].evaluate(native[6],d,0.);sf=native[7]
 print(j,dict(reference=float(mass[j]),current=float(stored[j,7]),native=native[3],sf_difference=sfc-sf,eta_difference=etac-native[5],p4=p.tolist(),deteta=d))
