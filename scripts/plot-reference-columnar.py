"""Overlay authoritative and columnar jet pt distributions from saved validated outputs."""
from pathlib import Path
import json
import numpy as np,awkward as ak,uproot
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
r=Path(__file__).resolve().parents[1];out=r/"results/plots";out.mkdir(exist_ok=True)
ref=uproot.open(r/"results/reference/reference.root")["analysis"]
col=uproot.open(r/"results/columnar-kinematics.root")
jer=next(k for k in col.keys(cycle=False) if k.startswith("JET_JER_EffectiveNP_1__") and k.endswith("up"))
states=[("NOSYS","Nominal"),("JET_EffectiveNP_Detector1__1up","JES detector component 1: +1 sigma"),(jer,"JER component 1: +1 sigma")]
plt.rcParams.update({"font.family":"DejaVu Sans","font.size":11,"axes.spines.top":False,"axes.spines.right":False})
fig=plt.figure(figsize=(15,7.3));gs=fig.add_gridspec(2,3,height_ratios=[3.5,1],hspace=.09,wspace=.29)
bins=np.arange(0,610,10);centers=(bins[1:]+bins[:-1])/2
report={}
for i,(name,title) in enumerate(states):
 a=ref["check_pt_"+name].array();b=col[name]["pt"].array()
 assert ak.all(ak.num(a)==ak.num(b))
 assert ak.all(ref["eventNumber"].array()==col[name]["eventNumber"].array())
 a=ak.to_numpy(ak.flatten(a)).astype(np.float64)/1000.;b=ak.to_numpy(ak.flatten(b)).astype(np.float64)/1000.
 ha,_=np.histogram(a,bins);hb,_=np.histogram(b,bins);valid=ha>0
 ax=fig.add_subplot(gs[0,i]);ratio=fig.add_subplot(gs[1,i],sharex=ax)
 ax.stairs(ha,bins,color="#175a9b",linewidth=2.1,label="AnalysisBase reference")
 ax.plot(centers[valid],hb[valid],linestyle="none",marker="o",markersize=4,markerfacecolor="none",markeredgewidth=1.1,color="#db6a22",label="Columnar + CorrectionLib")
 ax.set_yscale("log");ax.set_ylim(.7,max(ha.max(),hb.max())*2)
 ax.set_title(title,fontsize=12,pad=12);ax.set_ylabel("Jets / 10 GeV");ax.grid(axis="y",alpha=.16)
 ax.tick_params(labelbottom=False);ax.set_xlim(0,600)
 ratio.axhline(1,color="#175a9b",linewidth=1)
 ratio.plot(centers[valid],hb[valid]/ha[valid],linestyle="none",marker="o",markerfacecolor="none",markersize=3.5,color="#db6a22")
 ratio.set_ylim(.98,1.02);ratio.set_yticks([.98,1,1.02]);ratio.set_ylabel("Col. / ref.");ratio.set_xlabel(r"Jet $p_T$ [GeV]");ratio.grid(axis="y",alpha=.2)
 report[name]=dict(jets=len(a),reference_outside_plot=int(np.sum((a<0)|(a>600))),columnar_outside_plot=int(np.sum((b<0)|(b>600))),histogram_mismatched_bins=int(np.count_nonzero(ha!=hb)),max_per_jet_pt_difference_GeV=float(np.max(abs(a-b))))
 ax.text(.97,.95,"Same bin counts" if np.array_equal(ha,hb) else "See ratio panel",transform=ax.transAxes,ha="right",va="top",fontsize=10,color="#555555")
 if i==0:ax.legend(loc="lower left",frameon=False,fontsize=10)
fig.subplots_adjust(top=.79,bottom=.15,left=.065,right=.985)
fig.suptitle("ATLAS jet corrections: reference and columnar outputs",fontsize=19,x=.065,ha="left",y=.975)
fig.text(.065,.905,"Run-3 FullSim ttbar PHYSLITE  |  20,000 events  |  196,606 jets  |  AntiKt4EMPFlow",fontsize=12,color="#444444")
fig.text(.065,.857,"Blue: untouched AnalysisBase 25.2.100     Orange circles: shared CorrectionLib backend on PHYSLITE columns",fontsize=11,color="#444444")
fig.text(.065,.045,"Unweighted distributions; display range 0–600 GeV. Overlap is expected: the numerical comparisons pass.",fontsize=10,color="#444444")
fig.text(.065,.017,"Three representative states shown; full numerical validation covers 117 kinematic states plus two NNJVT weight variations.",fontsize=10,color="#444444")
for ext in ["png","pdf"]:fig.savefig(out/("reference-columnar-overlay."+ext),dpi=180,facecolor="white")
(out/"reference-columnar-overlay.json").write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
