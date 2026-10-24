from pathlib import Path
import os,sys,subprocess,shlex,json
r=Path('/home/gwatts/atlas/correctionlib_tests');w=Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib');os.chdir(r);os.sched_setaffinity(0,{min(os.sched_getaffinity(0))})
label='24-calculation-profile';base=r/'optimization-campaign'/label;base.mkdir(exist_ok=False)
s=(r/'c-nice/timing.py').read_text().replace('c-nice/results',f'optimization-campaign/{label}/runs')
s=s.replace(' stages=[];begin=time.perf_counter()', ' commands=[[cmd[0],"'+str(w/'detailed-profile-x2.py')+'"]+cmd[1:] for cmd in commands]\n stages=[];begin=time.perf_counter()')
(base/'worker.py').write_text(s)
with (base/'workflow.log').open('w') as log:subprocess.run([sys.executable,str(base/'worker.py'),'C','1'],check=True,stdout=log,stderr=subprocess.STDOUT)
print((base/'runs/C-1/results/detailed-timers.json').read_text(),flush=True)
flags=(r/'build/CMakeFiles/_atlas_corrections.dir/flags.make').read_text();includes=shlex.split(next(x.split('=',1)[1] for x in flags.splitlines() if x.startswith('CXX_INCLUDES =')))
link=shlex.split((r/'build/CMakeFiles/_atlas_corrections.dir/link.txt').read_text());libraries=link[next(i for i,x in enumerate(link) if x.startswith('-Wl,-rpath,')):]
subprocess.run([link[0],'-O2','-g','-std=c++23','-I'+str(r/'src'),*includes,str(w/'profile-native-x2.cxx'),'-o',str(base/'profile-native'),*libraries],check=True)
for mode in ('nn','efficiency','systematics'):
 with (base/f'{mode}.log').open('w') as log:subprocess.run(['valgrind','--tool=callgrind','--instr-atstart=no','--callgrind-out-file='+str(base/f'{mode}.callgrind'),str(base/'profile-native'),mode,str(base/f'runs/C-1/results/{mode}.bin')],check=True,stdout=log,stderr=subprocess.STDOUT)
 for inclusive in ('yes','no'):
  with (base/f'{mode}-{inclusive}.txt').open('w') as stream:subprocess.run(['callgrind_annotate','--inclusive='+inclusive,'--threshold=99','--auto=no',str(base/f'{mode}.callgrind.1')],check=True,stdout=stream)
 print(mode+' native profile complete',flush=True)
(base/'metadata.json').write_text(json.dumps({'git_head':subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),'cpu_affinity':sorted(os.sched_getaffinity(0)),'sample':'2048 evenly spaced jets from each actual native batch; full workflow unchanged','metric':'Callgrind instruction counts, not wall time','compile':'-O2 -g -std=c++23; current shared headers and installed libraries'},indent=2))
