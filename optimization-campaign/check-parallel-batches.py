from pathlib import Path
import sys
import numpy as np
root=Path('/home/gwatts/atlas/correctionlib_tests')
sys.path[:0]=[str(root/'install/python'),str(root/'python')]
from atlas_correctionlib.systematics import Systematics
from atlas_correctionlib.jvt import Jvt
from atlas_correctionlib import parallel
data=np.load(root/'results/jvt-inputs.npz')
n=16384
p4=data['p4'][:n];mu=data['mu'][:n];npv=data['npv'][:n];bjet=data['bjet'][:n]
rows=np.column_stack([p4,mu,npv,bjet])
sysengine=Systematics(root/'payloads');jvt=Jvt(root/'payloads',None)
pool=parallel._pool
assert parallel.workers==4
for name,varied in sysengine.variations(p4,mu,npv,bjet):
    assert np.array_equal(varied,sysengine.engine.batch_array(name,rows)[:,:4]),name
    args=(varied,data['sumpt'][:n],data['jvfcorr'][:n],data['haspv'][:n],data['ishs'][:n])
    actual=jvt.evaluate(*args)
    parallel._pool=None
    expected=jvt.evaluate(*args)
    parallel._pool=pool
    assert all(np.array_equal(a,b) for a,b in zip(actual,expected)),name
for sigma in (-1,1):
    args=(p4,data['sumpt'][:n],data['jvfcorr'][:n],data['haspv'][:n],data['ishs'][:n],sigma)
    actual=jvt.evaluate(*args)
    parallel._pool=None;expected=jvt.evaluate(*args);parallel._pool=pool
    assert all(np.array_equal(a,b) for a,b in zip(actual,expected))
print('Four-thread vs single-thread bitwise equality: 16,384 jets, 117 kinematic states and 119 JVT states PASS')
