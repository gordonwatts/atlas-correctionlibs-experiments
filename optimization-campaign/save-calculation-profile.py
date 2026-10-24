from pathlib import Path
import shutil,json,subprocess
r=Path('/home/gwatts/atlas/correctionlib_tests');w=Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib');base=r/'optimization-campaign/24-calculation-profile';dest=w/'optimization-results/24-calculation-profile';dest.mkdir(exist_ok=True)
for p in base.iterdir():
 if p.is_file() and p.suffix in ('.json','.txt','.log'):shutil.copy2(p,dest/p.name)
for name in ('nominal-comparison.json','systematics-comparison.json','jvt-comparison.json','detailed-timers.json','full-stage-timing.json'):
 p=base/'runs/C-1/results'/name;shutil.copy2(p,dest/name)
 if name.endswith('comparison.json'):assert json.loads(p.read_text())['passed']
shutil.copy2(base/'runs/C-1/timing.json',dest/'timing.json')
for name in ('detailed-profile-x2.py','profile-native-x2.cxx','run-calculation-profile.py','run-calculation-profile.sh','analyze-calculation-profile.py','analyze-calculation-profile.sh','save-calculation-profile.py'):shutil.copy2(w/name,r/'optimization-campaign'/name)
p=r/'.gitignore';s=p.read_text();s+='\n/optimization-campaign/**/profile-native\n';p.write_text(s)
subprocess.run(['git','add','.gitignore','optimization-campaign'],cwd=r,check=True)
subprocess.run(['git','commit','-q','-m','Capture native NNJVT, efficiency and grouped-systematic profiles plus exact feature reuse study'],cwd=r,check=True)
print('Saved profiles and all comparisons passed')
