from pathlib import Path
import json, os, shutil, statistics, subprocess, sys, time
import awkward as ak
import numpy as np
import uproot

root=Path('/home/gwatts/atlas/correctionlib_tests')
windows=Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib')
base=root/'a-stored-timing'
base.mkdir(exist_ok=False)
for name in ('a-stored.yaml','plot-a-stored.py','benchmark-a-stored.py'):
    shutil.copy2(windows/name,base/name)
sample=Path('/cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt/src/AsgExternal/Asg_Test/cmt/TestFileLiteR3MC.txt').read_text().strip()
baseline=json.loads((root/'c-nice/stored-timing/minimal-4/histogram.json').read_text())
records=[]
for iteration in range(1,5):
    work=base/f'run-{iteration}';work.mkdir()
    cmd=['CPRun.py','-i',sample,'-t',str(base/'a-stored.yaml'),'-o','output.root','-e','20000']
    start=time.perf_counter()
    with (work/'job.log').open('w') as log:
        subprocess.run(cmd,cwd=work,stdout=log,stderr=subprocess.STDOUT,check=True)
    job_seconds=time.perf_counter()-start
    start=time.perf_counter()
    with (work/'plot.log').open('w') as log:
        subprocess.run([sys.executable,str(base/'plot-a-stored.py')],cwd=work,env=dict(os.environ,MPLBACKEND='Agg'),stdout=log,stderr=subprocess.STDOUT,check=True)
    plot_seconds=time.perf_counter()-start
    counts=json.loads((work/'histogram.json').read_text())
    assert counts['counts_including_flow']==baseline['counts_including_flow']
    # Compare exported momentum and event identities to the stored inputs.
    with uproot.open(sample) as f, uproot.open(work/'output.root') as g:
        original=f['CollectionTree'].arrays(['AnalysisJetsAuxDyn.pt','AnalysisJetsAuxDyn.eta','EventInfoAuxDyn.eventNumber'],how=dict)
        result=g['analysis'].arrays(how=dict)
        for source,target in [('AnalysisJetsAuxDyn.pt','stored_pt_NOSYS'),('AnalysisJetsAuxDyn.eta','stored_eta_NOSYS')]:
            assert np.array_equal(ak.to_numpy(ak.num(original[source])),ak.to_numpy(ak.num(result[target])))
            assert np.array_equal(ak.to_numpy(ak.flatten(original[source])),ak.to_numpy(ak.flatten(result[target])))
        assert np.array_equal(ak.to_numpy(original['EventInfoAuxDyn.eventNumber']),ak.to_numpy(result['eventNumber']))
    record=dict(iteration=iteration,job_seconds=job_seconds,plot_seconds=plot_seconds,total_seconds=job_seconds+plot_seconds,passed=True,command=cmd)
    (work/'timing.json').write_text(json.dumps(record,indent=2));records.append(record)
    print(json.dumps(record),flush=True)
summary=dict(records=records,events=20000,jets=196606,discarded_run=1)
for key in ('job_seconds','plot_seconds','total_seconds'):
    values=[r[key] for r in records[1:]]
    summary[key]=dict(mean=statistics.mean(values),sd=statistics.stdev(values))
(base/'summary.json').write_text(json.dumps(summary,indent=2))
out=windows/'a-stored-timing';out.mkdir()
for name in ('summary.json','a-stored.yaml','plot-a-stored.py','benchmark-a-stored.py'):
    shutil.copy2(base/name,out/name)
for i in range(1,5):
    folder=out/f'run-{i}';folder.mkdir()
    for name in ('job.log','plot.log','histogram.json','timing.json','a-stored-jet-pt.png'):
        shutil.copy2(base/f'run-{i}'/name,folder/name)
print(json.dumps(summary,indent=2),flush=True)
