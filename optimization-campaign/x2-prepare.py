from pathlib import Path
import subprocess,shutil,json,hashlib
r=Path('/home/gwatts/atlas/correctionlib_tests')
e=r/'external/correctionlib'
out=r/'optimization-campaign/x2-checks';out.mkdir(exist_ok=True)
lib=r/'venv/lib/python3.13/site-packages/correctionlib/lib/libcorrectionlib.so'
shutil.copy2(lib,out/'original-libcorrectionlib.so')
(out/'original-library.json').write_text(json.dumps({'sha256':hashlib.sha256(lib.read_bytes()).hexdigest()},indent=2))
patch=r/'vendor-patches';patch.mkdir(exist_ok=True)
(patch/'existing-full-precision-json.patch').write_bytes(subprocess.check_output(['git','diff'],cwd=e))
subprocess.run(['git','-c','user.name=Gordon Watts','-c','user.email=gwatts@uw.edu','commit','-am','Preserve existing full-precision JSON parsing baseline'],cwd=e,check=True)
p=r/'.gitignore';p.write_text(p.read_text()+'\n/build-correctionlib-x2/\n')
