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
for i in range(3):
    with (base/f'python-stage-{i}.txt').open('w') as stream:
        stats=pstats.Stats(str(base/f'runs/C-1/profile-{i}.pstats'),stream=stream)
        stats.sort_stats('cumulative').print_stats(50)
        stats.sort_stats('tottime').print_stats(40)
data=np.load(root/'results/jvt-inputs.npz')
indices=np.linspace(0,len(data['p4'])-1,2048,dtype=int)
rows=np.column_stack([data['p4'][indices],data['mu'][indices],data['npv'][indices],data['bjet'][indices]])
rows.astype(np.float64).tofile(base/'rows.bin')
flags=(root/'build/CMakeFiles/_atlas_corrections.dir/flags.make').read_text()
includes=shlex.split(next(x.split('=',1)[1] for x in flags.splitlines() if x.startswith('CXX_INCLUDES =')))
link=shlex.split((root/'build/CMakeFiles/_atlas_corrections.dir/link.txt').read_text())
libraries=link[next(i for i,x in enumerate(link) if x.startswith('-Wl,-rpath,')):]
cpp='/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib/profile-systematics.cxx'
command=[link[0],'-O2','-g','-std=c++23','-I'+str(root/'src'),*includes,cpp,'-o',str(base/'profile-systematics'),*libraries]
subprocess.run(command,check=True)
with (base/'callgrind.log').open('w') as log:
    subprocess.run(['valgrind','--tool=callgrind','--instr-atstart=no','--callgrind-out-file='+str(base/'callgrind.out'),str(base/'profile-systematics'),str(base/'rows.bin')],check=True,stdout=log,stderr=subprocess.STDOUT)
for inclusive in ('yes','no'):
    with (base/f'callgrind-{inclusive}.txt').open('w') as output:
        subprocess.run(['callgrind_annotate','--inclusive='+inclusive,'--threshold=99','--auto=no',str(base/'callgrind.out.1')],check=True,stdout=output)
(base/'metadata.json').write_text(json.dumps(dict(cpu_affinity=sorted(os.sched_getaffinity(0)),threads=os.environ.get('ATLAS_CORRECTIONLIB_THREADS'),native_sample_jets=len(rows),git_head=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()),indent=2))
print(base,flush=True)
