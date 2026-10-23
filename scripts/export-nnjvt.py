import hashlib,json,shutil
from pathlib import Path
r=Path(__file__).resolve().parents[1]
base=Path("/cvmfs/atlas.cern.ch/repo/sw/database/GroupData/JetPileupTag/NNJvt/2022-03-22")
names={"network":"NNJVT.Network.graph.Offline.Nonprompt_All_MaxWeight.json","cuts":"NNJVT.Cuts.FixedEffPt.Offline.Nonprompt_All_MaxW.json"}
manifest={}
for name,filename in names.items():
 src=base/filename;dest=r/"payloads"/("nnjvt-"+name+".json")
 shutil.copyfile(src,dest)
 manifest[name]=dict(source=str(src),sha256=hashlib.sha256(src.read_bytes()).hexdigest(),bundle=dest.name)
(r/"payloads/nnjvt-manifest.json").write_text(json.dumps(manifest,indent=2))
