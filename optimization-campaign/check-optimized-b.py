from pathlib import Path
import json, subprocess, sys, time, hashlib
root=Path('/home/gwatts/atlas/correctionlib_tests')
work=root/'optimization-campaign/03-B-integration'
work.mkdir(exist_ok=False)
sample=Path('/cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt/src/AsgExternal/Asg_Test/cmt/TestFileLiteR3MC.txt').read_text().strip()
cmd=['CPRun.py','-i',sample,'-t',str(root/'config/reference.yaml'),'-o','output.root','-e','20000']
start=time.perf_counter()
with (work/'job.log').open('w') as log:
    subprocess.run(cmd,cwd=work,stdout=log,stderr=subprocess.STDOUT,check=True)
elapsed=time.perf_counter()-start
with (work/'comparison.log').open('w') as log:
    subprocess.run([sys.executable,str(root/'scripts/compare-atlas.py'),'--candidate',str(work/'output.root'),'--report',str(work/'comparison.json')],cwd=root,stdout=log,stderr=subprocess.STDOUT,check=True)
report=dict(wall_seconds=elapsed,passed=True,events=20000,configuration='config/reference.yaml',config_sha256=hashlib.sha256((root/'config/reference.yaml').read_bytes()).hexdigest(),command=cmd)
(work/'summary.json').write_text(json.dumps(report,indent=2));print(json.dumps(report),flush=True)
