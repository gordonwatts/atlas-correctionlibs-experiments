from pathlib import Path
import os,sys,subprocess,shlex,json,pstats
import numpy as np
root=Path('/home/gwatts/atlas/correctionlib_tests')
os.chdir(root)
os.sched_setaffinity(0,{min(os.sched_getaffinity(0))})
label=sys.argv[1]
base=root/'optimization-campaign'/label
base.mkdir(exist_ok=False)
source=(root/'c-nice/timing.py').read_text().replace('c-nice/results',f'optimization-campaign/{label}/runs')
source=source.replace(' stages=[];begin=time.perf_counter()', ' commands=[[cmd[0],"-m","cProfile","-o",str(work/("profile-"+str(i)+".pstats"))]+cmd[1:] for i,cmd in enumerate(commands)]\n stages=[];begin=time.perf_counter()')
(base/'worker.py').write_text(source)
with (base/'python-profile.log').open('w') as log:
    subprocess.run([sys.executable,str(base/'worker.py'),'C','1'],check=True,stdout=log,stderr=subprocess.STDOUT)
for i in range(1):
    with (base/f'python-stage-{i}.txt').open('w') as stream:
        stats=pstats.Stats(str(base/f'runs/C-1/profile-{i}.pstats'),stream=stream)
        stats.sort_stats('cumulative').print_stats(50)
        stats.sort_stats('tottime').print_stats(40)
entries=[]
stats=pstats.Stats(str(base/'runs/C-1/profile-0.pstats'))
for (filename,line,name),(primitive,calls,own,cumulative,callers) in stats.stats.items():
    entries.append(dict(file=filename,line=line,name=name,calls=calls,self_seconds=own,cumulative_seconds=cumulative))
(base/'functions.json').write_text(json.dumps(sorted(entries,key=lambda row:row['cumulative_seconds'],reverse=True),indent=2))
(base/'metadata.json').write_text(json.dumps(dict(cpu_affinity=sorted(os.sched_getaffinity(0)),threads=os.environ.get('ATLAS_CORRECTIONLIB_THREADS'),git_head=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),profile='cProfile; timings include instrumentation overhead; native calls opaque'),indent=2))
print(base,flush=True)
