from pathlib import Path
import hashlib,json,os,subprocess,sys,time
root=Path('/home/gwatts/atlas/correctionlib_tests')
os.sched_setaffinity(0,{min(os.sched_getaffinity(0))})
work=root/'optimization-campaign/x2-B-integration'
work.mkdir(exist_ok=False)
sample=Path('/cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt/src/AsgExternal/Asg_Test/cmt/TestFileLiteR3MC.txt').read_text().strip()
cmd=['CPRun.py','-i',sample,'-t',str(root/'config/reference.yaml'),'-o','output.root','-e','20000']
start=time.perf_counter()
with (work/'job.log').open('w') as log:
    child=subprocess.Popen(cmd,cwd=work,stdout=log,stderr=subprocess.STDOUT)
    observed=set()
    mappings=''
    while child.poll() is None:
        pending=[child.pid]
        while pending:
            pid=pending.pop()
            try:
                pending.extend(int(x) for x in Path(f'/proc/{pid}/task/{pid}/children').read_text().split())
                maps=Path(f'/proc/{pid}/maps').read_text()
                lines=[line for line in maps.splitlines() if 'build-overlay' in line or 'libcorrectionlib.so' in line]
                if lines:
                    observed.update(lines)
                    mappings='\n'.join(sorted(observed))+'\n'
            except FileNotFoundError:
                pass
        time.sleep(.5)
    if child.returncode:raise subprocess.CalledProcessError(child.returncode,cmd)
elapsed=time.perf_counter()-start
lib=root/'venv/lib/python3.13/site-packages/correctionlib/lib/libcorrectionlib.so'
assert str(lib) in mappings, 'Expected rebuilt CorrectionLib was not observed'
(work/'loaded-libraries.txt').write_text(mappings)
assert 'libJetUncertaintiesLib.so' in mappings, 'Did not observe the rebuilt uncertainty library in the B job'
with (work/'comparison.log').open('w') as log:
    subprocess.run([sys.executable,str(root/'scripts/compare-atlas.py'),'--candidate',str(work/'output.root'),'--report',str(work/'comparison.json')],cwd=root,check=True,stdout=log,stderr=subprocess.STDOUT)
report=dict(library_sha256=hashlib.sha256(lib.read_bytes()).hexdigest(),wall_seconds=elapsed,passed=True,events=20000,cpu_affinity=sorted(os.sched_getaffinity(0)),configuration='config/reference.yaml',config_sha256=hashlib.sha256((root/'config/reference.yaml').read_bytes()).hexdigest(),command=cmd)
(work/'summary.json').write_text(json.dumps(report,indent=2));print(json.dumps(report),flush=True)


