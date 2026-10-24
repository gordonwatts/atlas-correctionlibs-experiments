from pathlib import Path
import json,numpy as np
from atlas_correctionlib.jvt import Jvt
r=Path('/home/gwatts/atlas/correctionlib_tests')
x=np.load(r/'optimization-campaign/15-paired-systematics/runs/C-2/results/jvt-inputs.npz')
inputs=[x[name][:512].copy() for name in ('p4','sumpt','jvfcorr','haspv','ishs')]
cached=Jvt(r/'payloads',None);fresh=Jvt(r/'payloads',None)
def check(args,sigma=0):
 fresh._anchor=None
 a=cached.evaluate(*args,sigma=sigma);b=fresh.evaluate(*args,sigma=sigma)
 assert all(np.array_equal(u,v) for u,v in zip(a,b))
 return a
first=check(inputs)
for value in first:value[:]=0
check(inputs) # Returned arrays cannot poison cache.
for i in range(5):
 altered=[a.copy() for a in inputs]
 if altered[i].dtype==bool:altered[i][0]=~altered[i][0]
 else:altered[i][0]+=1
 check(altered)
 check(inputs)
check(inputs,1);check(inputs,-1)
inputs[0][0,0]+=500
check(inputs) # In-place input mutation cannot hit stale anchor.
cached.nn=fresh.nn
check(inputs)
check([a[:0] for a in inputs])
report={'passed':True,'jets':512,'checks':['exact repeat','returned-array mutation','each effective input changed','sigma up/down','in-place input mutation','engine replacement','empty batch']}
(r/'optimization-campaign/x2-checks/jvt-cache-tests.json').write_text(json.dumps(report,indent=2))
print(report)
