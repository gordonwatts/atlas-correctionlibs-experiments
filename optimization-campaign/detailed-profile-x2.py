import sys,time,json,runpy
from pathlib import Path
import numpy as np
import atlas_correctionlib.jvt as jm
import atlas_correctionlib.systematics as sm
from atlas_jet_corrections import JetCorrections
root=Path.cwd();sections={};captures={};original=jm.parallel_batch

def add(name,seconds):
 item=sections.setdefault(name,{'calls':0,'seconds':0.});item['calls']+=1;item['seconds']+=seconds

def measured(function,rows,*args,**kwargs):
 name='nn' if getattr(function,'__self__',None) is not None else 'efficiency'
 start=time.perf_counter();result=original(function,rows,*args,**kwargs);add(name,time.perf_counter()-start)
 indices=np.linspace(0,len(rows)-1,min(2048,len(rows)),dtype=int)
 # Capture the actual evaluated batches, after cache decisions, for native replay.
 sigma=function.__closure__[-1].cell_contents if name=='efficiency' else 0
 if name=='efficiency':
  closed=dict(zip(function.__code__.co_freevars,[x.cell_contents for x in function.__closure__]));sigma=closed['sigma']
 captures.setdefault(name,[]).append((np.asarray(rows[indices],dtype=np.float64),int(sigma)))
 return result
jm.parallel_batch=measured
old_evaluate=jm.Jvt.evaluate
def evaluate(self,*args,**kwargs):
 start=time.perf_counter()
 try:return old_evaluate(self,*args,**kwargs)
 finally:add('jvt_total',time.perf_counter()-start)
jm.Jvt.evaluate=evaluate
old_array_equal=jm.np.array_equal
# Measure only equality checks executed from the JVT implementation.
def eq(a,b,*args,**kwargs):
 if sys._getframe(1).f_code.co_filename.endswith('/jvt.py'):
  start=time.perf_counter();result=old_array_equal(a,b,*args,**kwargs);add('jvt_cache_equality',time.perf_counter()-start);return result
 return old_array_equal(a,b,*args,**kwargs)
jm.np.array_equal=eq
old_sys=sm.parallel_batch
def systematic(function,rows,*args,**kwargs):
 start=time.perf_counter();result=old_sys(function,rows,*args,**kwargs);add('systematics_native_batch',time.perf_counter()-start)
 if 'systematics' not in captures:
  indices=np.linspace(0,len(rows)-1,min(2048,len(rows)),dtype=int)
  captures['systematics']=[(np.asarray(rows[indices],dtype=np.float64),0)]
 return result
sm.parallel_batch=systematic
sys.argv=sys.argv[1:]
runpy.run_path(sys.argv[0],run_name='__main__')
for name,batches in captures.items():
 with (root/'results'/f'{name}.bin').open('wb') as stream:
  for rows,sigma in batches:
   np.asarray([len(rows),rows.shape[1],sigma],dtype=np.int64).tofile(stream);rows.tofile(stream)
(root/'results/detailed-timers.json').write_text(json.dumps(sections,indent=2))
