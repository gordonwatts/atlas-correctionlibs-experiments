"""Compile the pinned MC23 nominal response functions. No event outputs are inputs."""
import json,math,gzip
from pathlib import Path
import ROOT,correctionlib
from .payloads import Sources,vector,centres
from .nodes import *
ROOT.gROOT.SetBatch(True)
AREA="JetCalibTools/CalibArea-00-04-83/"
CONFIG=AREA+"CalibrationConfigs/AntiKt4EMPFlow_MC23a_PreRecR22_Phase2_CalibConfig_ResPU_EtaJES_GSC_241208_InSitu.config"
def area_path(p):
    if not p.startswith("JetCalibTools/"): raise ValueError(p)
    return p.replace("JetCalibTools/",AREA,1)
def compile_bundle(out):
    src=Sources();env=src.env(CONFIG);cs=[];objects={}
    def add(name,inputs,node,source):
        cs.append(correction(name,inputs,node));objects[name]=source
    if env.GetValue("DoSequentialResidual",False) or env.GetValue("Do3DCorrection",False):
        raise ValueError("unsupported pileup mode")
    residual=area_path(env.GetValue("ResidualOffset.CalibFile",""));src.merge(env,residual)
    prefix=env.GetValue("ResidualOffsetCorrection.Name","")
    eta=vector(env,prefix+".AbsEtaBins")
    for kind in ["Mu","NPV"]:
        coeff=vector(env,prefix+"."+kind+"Term.AntiKt4EMPFlow")
        add("residual_"+kind.lower(),[("abseta","real")],piecewise_coeff(eta,coeff,"abseta"),
            dict(file=residual,key=prefix+"."+kind+"Term.AntiKt4EMPFlow"))
    add("pileup_scale",[("pt","real"),("area","real"),("rho","real"),("offset","real")],
        formula("((x-y*z-t)>=0)*(x-y*z-t)/x+((x-y*z-t)<0)*10/x",["pt","area","rho","offset"]),dict(implementation="JetPileupCorrection"))
    constants=area_path(env.GetValue("AbsoluteJES.CalibFile",""));src.merge(env,constants)
    if not env.GetValue("AntiKt4EMPFlow.isSpline",False): raise ValueError("requires histogram JES mode")
    if env.GetValue("LowPtJESExtrapolationMethod",0)!=1: raise ValueError("requires linear low-pt extrapolation")
    if env.GetValue("ApplyMassCorrection",False): raise ValueError("mass correction not supported")
    histpath=area_path(env.GetValue("AbsoluteJES.CalibHists",""));f=src.root(histpath)
    histlist=f.Get("etaJes")
    eta=vector(env,"JES.EtaBins") if env.GetValue("JES.EtaBins","") else [0.1*i-4.5 for i in range(91)]
    minpt=env.GetValue("AntiKt4EMPFlow.MinPtForETAJES",10.)
    responses=[];lowresponses=[];etacorr=[]
    for i in range(len(eta)-1):
        h=histlist.At(i)
        if not h.InheritsFrom("TH1"): raise TypeError(h.ClassName())
        xs=centres(h.GetXaxis());ys=[h.GetBinContent(j) for j in range(1,h.GetNbinsX()+1)]
        node=linear(xs,ys,"energy",inverse=True)
        freeze=vector(env,"EmaxJES.AntiKt4EMPFlow_Bin"+str(i))[0]
        if env.GetValue("AntiKt4EMPFlow.FreezeJEScorrectionatHighE",False) and freeze!=-1:
            node=transform("energy",formula("min(x,[0])",["energy"],[freeze]),node)
        responses.append(node)
        cutoff=minpt*math.cosh(eta[i]);b=h.FindBin(cutoff)
        slope=(h.GetBinContent(b+1)-h.GetBinContent(b))/(h.GetBinCenter(b+1)-h.GetBinCenter(b))
        lowresponses.append(formula("1/([0]+[1]*(x-[2]))",["energy"],[h.Interpolate(cutoff),slope,cutoff]))
        coeff=vector(env,"EtaCorr.AntiKt4EMPFlow_Bin"+str(i))
        expr="-("+ "+".join("["+str(j)+"]*log(x)^"+str(j) for j in range(len(coeff))) +")"
        etacorr.append(formula(expr,["energy"],coeff))
    f.Close()
    # Low-pt decision depends on actual detector eta, not eta-bin lower edge.
    high=binning("detector_eta",eta,responses,"clamp")
    low=binning("detector_eta",eta,lowresponses,"clamp")
    select=binning("lowpt",[-1.,0.5,2.],[high,low])
    node=transform("lowpt",formula("x/cosh(y)<[0]",["energy","detector_eta"],[minpt]),select)
    add("jes_scale",[("energy","real"),("detector_eta","real"),("lowpt","real")],node,dict(file=histpath,object="etaJes",constants=constants))
    node=binning("detector_eta",eta,etacorr,"clamp")
    node=transform("energy",formula("min(max(x,8*cosh(y)),2500)",["energy","detector_eta"]),node)
    add("eta_shift",[("energy","real"),("detector_eta","real")],node,dict(file=constants,key="EtaCorr.*",defaults=dict(min_pt=8,max_energy=2500)))
    gscpath=area_path(env.GetValue("GSCFactorsFile",""));g=src.root(gscpath)
    if env.GetValue("TurnOffTrackCorrections",False): raise ValueError("track turnoff not supported")
    if env.GetValue("GSCDepth","")!="trackWIDTH" or not env.GetValue("nTrkwTrk4PFlow",False): raise ValueError("unexpected GSC sequence")
    specs=[("chargedFraction",27),("Tile0",17),("EM3",env.GetValue("EM3MaxEtaBin",35)),("nTrk",25),("trackWIDTH",25)]
    rawkeys=[k.GetName() for k in g.GetListOfKeys()]
    keys=[k for i,k in enumerate(rawkeys) if i==0 or rawkeys[i-1] not in k]
    for prop,limit in specs:
        selected=[k for k in keys if prop in k and "AntiKt4EMPFlow" in k][:limit]
        if not selected: raise ValueError(("no histograms",prop,keys[:5]))
        nodes=[]
        for key in selected:
            h=g.Get(key)
            xs=centres(h.GetXaxis());ys=centres(h.GetYaxis())
            vals=[[h.GetBinContent(i,j) for j in range(1,h.GetNbinsY()+1)] for i in range(1,h.GetNbinsX()+1)]
            nodes.append(bilinear(xs,ys,vals,"pt",prop,inverse=True))
        # int(absEta/0.1) in ATLAS; use transformed index to retain boundary arithmetic.
        node=binning("eta_index",list(range(len(nodes)+1)),nodes,1.)
        node=transform("eta_index",formula("x/0.1",["abseta"]),node)
        # Tile0 accepts exactly zero; other properties return unity at <=0.
        threshold=0. if prop=="Tile0" else math.nextafter(0.,math.inf)
        node=binning(prop,["-inf",threshold,"inf"],[1.,node])
        add("gsc_"+prop,[("pt","real"),("abseta","real"),("eta_index","real"),(prop,"real")],node,dict(file=gscpath,objects=selected))
    g.Close()
    inputs=[("pt","real"),("abseta","real"),("eta_index","real")]+[(p,"real") for p,_ in specs]
    compound=dict(name="gsc",inputs=[dict(name=n,type=t) for n,t in inputs],output=dict(name="scale",type="real"),inputs_update=["pt"],input_op="*",output_op="*",stack=["gsc_"+p for p,_ in specs])
    bundle=dict(schema_version=2,corrections=cs,compound_corrections=[compound])
    # Parse with the actual evaluator before publishing.
    correctionlib.CorrectionSet.from_string(json.dumps(bundle))
    out=Path(out);out.mkdir(parents=True,exist_ok=True)
    with gzip.open(out/"nominal.json.gz","wt") as fp:json.dump(bundle,fp)
    settings={k:env.GetValue(k,d) for k,d in [("OffsetCorrection.DefaultMuRef",0.),("OffsetCorrection.DefaultNPVRef",1.),("MuScaleFactor",1.),("ApplyNPVBeamspotCorrection",False),("RhoKey",""),("GSCDepth","")]}
    (out/"nominal-manifest.json").write_text(json.dumps(dict(status="compiled-not-end-to-end-validated",sources=src.files,corrections=objects,settings=settings,release="25.2.100"),indent=2))
    print("Compiled",len(cs),"corrections and GSC compound")
if __name__=="__main__":
    import sys
    compile_bundle(sys.argv[1] if len(sys.argv)>1 else "payloads")
