from pathlib import Path
import json,sys,subprocess,hashlib
import _declarative,ROOT
r=Path(__file__).resolve().parents[2];d=r/"path-d"
maps=Path("/proc/self/maps").read_text().splitlines()
libraries=sorted(set(x.split()[-1] for x in maps if "libcorrectionlib.so" in x))
assert libraries and all("/path-d/build/correctionlib/" in x for x in libraries),libraries
p=r/"external/correctionlib-d"
subprocess.run(["patch","--dry-run","-R","-p1","-i",str(d/"patches/correctionlib-graph-v1.patch")],cwd=p,check=True)
report=dict(passed=True,evaluator=_declarative.evaluator_version,module=_declarative.__file__,correctionlib_libraries=libraries,root=ROOT.gROOT.GetVersion(),python=sys.version,compiler=subprocess.check_output(["g++","--version"],text=True).splitlines()[0],fork_patch_reverse_dry_run=True,precision_patch_sha256=hashlib.sha256((r/"patches/correctionlib-full-precision-json.patch").read_bytes()).hexdigest(),extension_patch_sha256=hashlib.sha256((d/"patches/correctionlib-graph-v1.patch").read_bytes()).hexdigest())
(d/"results/runtime.json").write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
