from pathlib import Path
import os,sys,subprocess
r=Path('/home/gwatts/atlas/correctionlib_tests');os.chdir(r)
os.sched_setaffinity(0,{min(os.sched_getaffinity(0))})
label='23-final-scoped-profile';base=r/'optimization-campaign'/label;base.mkdir(exist_ok=False)
s=(r/'c-nice/timing.py').read_text().replace('c-nice/results',f'optimization-campaign/{label}/runs')
s=s.replace(' stages=[];begin=time.perf_counter()', ' commands=[[cmd[0],"/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib/scoped-profile-x2.py"]+cmd[1:] for cmd in commands]\n stages=[];begin=time.perf_counter()')
(base/'worker.py').write_text(s)
with (base/'run.log').open('w') as log:subprocess.run([sys.executable,str(base/'worker.py'),'C','1'],check=True,stdout=log,stderr=subprocess.STDOUT)
print((base/'runs/C-1/results/scoped-profile.json').read_text())
