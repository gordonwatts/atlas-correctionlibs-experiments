"""Create the isolated optional-extension fork from the preserved precision-patched base."""
from pathlib import Path
import shutil,subprocess,hashlib
r=Path(__file__).resolve().parents[2]
base=r/"external/correctionlib";target=r/"external/correctionlib-d"
if not target.exists():
 shutil.copytree(base,target,ignore=shutil.ignore_patterns(".git","build","__pycache__"))
 subprocess.run(["patch","-p1","-i",str(r/"path-d/patches/correctionlib-graph-v1.patch")],cwd=target,check=True)
assert (base/"src/correction.cc").read_bytes()==(target/"src/correction.cc").read_bytes()
assert b"kParseFullPrecisionFlag" in (target/"src/correction.cc").read_bytes()
print("Private CorrectionLib fork and full-precision parser verified")
