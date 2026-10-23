from pathlib import Path
import subprocess,json,hashlib,sys,platform
r=Path(__file__).resolve().parents[1]
def command(*args):return subprocess.check_output(args,text=True).strip()
local=command(sys.executable,"-m","pip","list","--local","--format=freeze").splitlines()
lock=[s for s in local if not s.lower().startswith("correctionlib==")]
(r/"config/requirements-wheels.lock").write_text("\n".join(lock)+"\n")
(r/"external/python-packages.txt").write_text(command(sys.executable,"-m","pip","freeze")+"\n")
meta=dict(python=sys.version,platform=platform.platform(),compiler=command("g++","--version").splitlines()[0],root_version=command("root-config","--version"),correctionlib_commit=command("git","-C",str(r/"external/correctionlib"),"rev-parse","HEAD"),correctionlib_submodules=command("git","-C",str(r/"external/correctionlib"),"submodule","status"),packages_local=local,analysisbase="25.2.100",athena_smoke="25.0.76")
(r/"config/environment.json").write_text(json.dumps(meta,indent=2))
print("Recorded",len(lock),"pinned wheel requirements")
