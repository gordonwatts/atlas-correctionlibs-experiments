import sys,time,json,functools,runpy
from pathlib import Path
from collections import defaultdict
import uproot
from atlas_jet_corrections import JetCorrections
from atlas_correctionlib.reference import ReferenceReader
from atlas_correctionlib.systematics import Systematics
from atlas_correctionlib.jvt import Jvt
measurements=defaultdict(lambda:{'calls':0,'seconds':0.})
def timed(label,function):
 @functools.wraps(function)
 def wrapper(*args,**kwargs):
  start=time.perf_counter()
  try:return function(*args,**kwargs)
  finally:
   measurements[label]['seconds']+=time.perf_counter()-start
   measurements[label]['calls']+=1
 return wrapper
ReferenceReader.read=timed('A reference reads',ReferenceReader.read)
JetCorrections.load=staticmethod(timed('Nominal tool load',JetCorrections.load))
JetCorrections.evaluate=timed('Nominal input preparation and native evaluation',JetCorrections.evaluate)
Systematics.__init__=timed('Systematic tool load',Systematics.__init__)
Jvt.__init__=timed('JVT tool load',Jvt.__init__)
Jvt.evaluate=timed('JVT evaluate including array preparation and exact cache',Jvt.evaluate)
uproot.writing.writable.WritableDirectory.__setitem__=timed('ROOT output assignment',uproot.writing.writable.WritableDirectory.__setitem__)
original=Systematics.variations
def variations(self,*args,**kwargs):
 iterator=iter(original(self,*args,**kwargs))
 while True:
  start=time.perf_counter()
  try:value=next(iterator)
  except StopIteration:
   measurements['Systematic variation production']['seconds']+=time.perf_counter()-start
   return
  measurements['Systematic variation production']['seconds']+=time.perf_counter()-start
  measurements['Systematic variation production']['calls']+=1
  yield value
Systematics.variations=variations
sys.argv=sys.argv[1:]
start=time.perf_counter()
try:runpy.run_path(sys.argv[0],run_name='__main__')
finally:
 Path('results/scoped-profile.json').write_text(json.dumps({'run_path_seconds':time.perf_counter()-start,'sections':dict(measurements),'note':'Imports precede run_path timer; outer worker wall time includes imports. Section timings are wall time and are not cProfile timings.'},indent=2))
