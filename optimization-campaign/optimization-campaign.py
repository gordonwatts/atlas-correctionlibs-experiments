"""Run one isolated optimization checkpoint; preserve the original full workflow."""
from pathlib import Path
import json, os, statistics, subprocess, sys
root=Path('/home/gwatts/atlas/correctionlib_tests')
label=sys.argv[1]
base=root/'optimization-campaign'/label
base.mkdir(parents=True,exist_ok=False)
source=(root/'c-nice/timing.py').read_text().replace('c-nice/results',f'optimization-campaign/{label}/runs')
(base/'worker.py').write_text(source)
records=[]
for i in range(1,4):
    with (base/f'run-{i}.log').open('w') as log:
        subprocess.run([sys.executable,str(base/'worker.py'),'C',str(i)],cwd=root,stdout=log,stderr=subprocess.STDOUT,check=True)
    record=json.loads((base/f'runs/C-{i}/timing.json').read_text())
    for stage in ('nominal','systematics','jvt'):
        assert json.loads((base/f'runs/C-{i}/results/{stage}-comparison.json').read_text())['passed']
    records.append(record)
    print(label,i,round(record['wall_seconds'],3),'PASS',flush=True)
values=[r['wall_seconds'] for r in records[1:]]
report=dict(label=label,mean=statistics.mean(values),sd=statistics.stdev(values),records=records,discarded_run=1,git_head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip(),passed=True)
(base/'summary.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report),flush=True)
