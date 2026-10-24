from pathlib import Path
import json,subprocess,shutil
r=Path('/home/gwatts/atlas/correctionlib_tests')
w=Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib')
b=r/'optimization-campaign/x2-B-integration'
summary=json.loads((b/'summary.json').read_text());assert summary['passed']
expected=json.loads((r/'optimization-campaign/20-flat-adapters/implementation.json').read_text())
assert summary['library_sha256']==expected['library_sha256']
assert summary['config_sha256']=='4d84aa237b6a1796c05e9a38fa341e89993f1b8c582bc99da7b1624b4ed46ca8'
subprocess.run(['git','diff','--exit-code','38935839d523edfad166fcb740fdcb79deb8ed88','--','src','python','scripts','c-nice'],cwd=r,check=True)
(r/'optimization-campaign/x2-checks/selected-source.json').write_text(json.dumps({'selected_commit':'38935839d523edfad166fcb740fdcb79deb8ed88','implementation_matches_selected_commit':True,'chunk_size':1000},indent=2))
for p in w.glob('x2-*'):
 if p.suffix in ('.py','.sh'):shutil.copy2(p,r/'optimization-campaign'/p.name)
for name in ('x2-checks','x2-B-integration'):
 out=w/'optimization-results'/name;out.mkdir(exist_ok=True)
 for p in (r/'optimization-campaign'/name).iterdir():
  if p.is_file() and p.suffix in ('.json','.log','.txt'):shutil.copy2(p,out/p.name)
subprocess.run(['git','add','optimization-campaign'],cwd=r,check=True)
subprocess.run(['git','commit','-q','-m','Validate final x2 campaign: unchanged B configuration, A comparisons and raw-byte C parity'],cwd=r,check=True)
print(subprocess.check_output(['git','rev-parse','HEAD'],cwd=r,text=True).strip())


