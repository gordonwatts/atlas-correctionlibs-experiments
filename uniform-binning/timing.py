from pathlib import Path
import json,os,statistics,subprocess,sys
root=Path('/home/gwatts/atlas/correctionlib_tests')
label=sys.argv[1]
base=root/'optimization-campaign'/label
base.mkdir(exist_ok=True,parents=True)
assert not (base/'runs').exists()
os.sched_setaffinity(0,{min(os.sched_getaffinity(0))})
source=(root/'c-nice/timing.py').read_text().replace('c-nice/results',f'optimization-campaign/{label}/runs')
(base/'worker.py').write_text(source)
records=[]
for i in range(1,4):
    subprocess.run([sys.executable,str(base/'worker.py'),'C',str(i)],cwd=root,check=True)
    records.append(json.loads((base/f'runs/C-{i}/timing.json').read_text()))
summary=dict(label=label,mean=statistics.mean(r['wall_seconds'] for r in records[1:]),sd=statistics.stdev(r['wall_seconds'] for r in records[1:]),records=records,passed=True,cpu_affinity=sorted(os.sched_getaffinity(0)))
(base/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
print(json.dumps(summary),flush=True)
