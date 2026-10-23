"""Make the replacement's runtime bundle independent of original calibration files."""
from pathlib import Path
import json,hashlib,shutil,subprocess
r=Path(__file__).resolve().parents[1];out=r/"payloads"
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
for filename in ["nominal-manifest.json","uncertainty-manifest.json"]:
 manifest=json.loads((out/filename).read_text())
 for source,record in manifest["sources"].items():
  src=Path(source)
  if src.suffix not in (".config",".txt"):continue
  if sha(src)!=record["sha256"]:raise RuntimeError("Source changed: "+source)
  target=out/record["logical_path"];target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,target)
nn=json.loads((out/"nnjvt-manifest.json").read_text())
for item in nn.values():
 src=Path(item["source"]);logical=src.relative_to("/cvmfs/atlas.cern.ch/repo/sw/database/GroupData")
 if sha(src)!=item["sha256"]:raise RuntimeError("NNJVT source changed")
 target=out/logical;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,target)
files={str(p.relative_to(out)):dict(sha256=sha(p),size=p.stat().st_size) for p in sorted(out.rglob("*")) if p.is_file() and p.name!="bundle-manifest.json"}
manifest=dict(correctionlib_base_commit=subprocess.check_output(["git","-C",str(r/"external/correctionlib"),"rev-parse","HEAD"],text=True).strip(),required_patch="patches/correctionlib-full-precision-json.patch",patch_sha256=sha(r/"patches/correctionlib-full-precision-json.patch"),schema_version=2,files=files)
(out/"bundle-manifest.json").write_text(json.dumps(manifest,indent=2))
print("Packaged",len(files),"runtime files; original calibration ROOT files are not included.")
