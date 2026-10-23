"""Export selected uncertainty/JVT source tables and configuration, without claiming full evaluator parity."""
import json,gzip,re
from pathlib import Path
import ROOT,correctionlib
from .payloads import Sources,centres,edges
from .nodes import *
BASE="JetUncertainties/CalibArea-08/"
CFG=BASE+"rel22/Winter2025_PreRec/R4_CategoryReduction_FullJER.config"
def env_dict(env):
    return {x.GetName():x.GetValue() for x in env.GetTable()}
def hist_node(h,mode,quantity="content"):
    if h.GetDimension()>2: raise ValueError("unsupported histogram dimension")
    value=h.GetBinContent if quantity=="content" else h.GetBinError
    if h.GetDimension()==1:
        values=[float(value(i)) for i in range(1,h.GetNbinsX()+1)]
        return linear(centres(h.GetXaxis()),values,"pt") if mode in ("Full","OnlyX") else binning("pt",edges(h.GetXaxis()),values,"clamp")
    xs,ys=centres(h.GetXaxis()),centres(h.GetYaxis())
    values=[[float(value(i,j)) for j in range(1,h.GetNbinsY()+1)] for i in range(1,h.GetNbinsX()+1)]
    if mode=="Full":return bilinear(xs,ys,values,"pt","eta")
    if mode=="OnlyX":
        return binning("eta",edges(h.GetYaxis()),[linear(xs,[r[j] for r in values],"pt") for j in range(len(ys))],"clamp")
    if mode=="None":
        return binning("pt",edges(h.GetXaxis()),[binning("eta",edges(h.GetYaxis()),row,"clamp") for row in values],"clamp")
    raise ValueError(mode)
def compile_bundle(out):
    src=Sources();env=src.env(CFG);settings=env_dict(env);corrections=[];mapping={}
    # Retain original component/group recipes explicitly; these are not yet a full implementation.
    components={};groups={}
    for key,v in settings.items():
        m=re.fullmatch(r"JES(Component|Group)\.(\d+)\.(.+)",key)
        if m:(components if m[1]=="Component" else groups).setdefault(m[2],{})[m[3]]=v
    paths=[BASE+env.GetValue("UncertaintyRootFile",""),BASE+env.GetValue("AnalysisRootFile",""),"JetJvtEfficiency/May2024/NNJvtSFFile_Run3_EMPFlow.root"]
    inventory={}
    for file_index,path in enumerate(paths):
        f=src.root(path);seen=set()
        for key in f.GetListOfKeys():
            name=key.GetName()
            if name in seen:continue
            seen.add(name)
            obj=f.Get(name)
            inventory[path+"::"+name]=dict(classname=obj.ClassName())
            if not obj.InheritsFrom("TH1"):
                inventory[path+"::"+name]["converted"]=False
                continue
            inventory[path+"::"+name]["bounds"]=[obj.GetXaxis().GetXmin(),obj.GetXaxis().GetXmax(),obj.GetYaxis().GetXmin(),obj.GetYaxis().GetXmax()]
            if file_index==0 and "AntiKt4EMPFlow" not in name:continue
            if obj.GetDimension()>2:raise ValueError((path,name,obj.ClassName()))
            modes=["None"] if file_index==2 else ["Full","OnlyX","None"]
            for mode in modes:
                for quantity in (["content","error"] if file_index==2 else ["content"]):
                    cid=f"file{file_index}__{name}__{mode}__{quantity}"
                    inputs=[("pt","real")]+([("eta","real")] if obj.GetDimension()==2 else [])
                    corrections.append(correction(cid,inputs,hist_node(obj,mode,quantity)))
                    mapping[cid]=dict(file=path,object=name,mode=mode,quantity=quantity)
            inventory[path+"::"+name]["converted"]=True
        f.Close()
    bundle=dict(schema_version=2,corrections=corrections)
    correctionlib.CorrectionSet.from_string(json.dumps(bundle))
    out=Path(out);out.mkdir(exist_ok=True)
    with gzip.open(out/"uncertainty-tables.json.gz","wt") as f:json.dump(bundle,f)
    (out/"uncertainty-manifest.json").write_text(json.dumps(dict(status="tables-only-not-complete-systematic-evaluator",sources=src.files,settings=settings,components=components,groups=groups,mapping=mapping,inventory=inventory),indent=2))
    print("Exported",len(corrections),"table corrections;",len(components),"component recipes;",len(groups),"group recipes")
if __name__=="__main__":
    import sys
    compile_bundle(sys.argv[1] if len(sys.argv)>1 else "payloads")
