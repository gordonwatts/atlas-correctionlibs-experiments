from pathlib import Path
import sys
import numpy as np
root=Path('/home/gwatts/atlas/correctionlib_tests')
sys.path[:0]=[str(root/'install/python'),str(root/'python')]
from _atlas_corrections import JetSystematics
from atlas_correctionlib.jvt import Jvt
data=np.load(root/'results/jvt-inputs.npz')
rows=np.column_stack([data['p4'],data['mu'],data['npv'],data['bjet']])[:1024]
engine=JetSystematics(str(root/'payloads/uncertainty-tables.json.gz'),str(root/'payloads/uncertainty-manifest.json'))
for state in engine.names():
    expected=np.asarray(engine.batch(state,rows.tolist()))
    actual=engine.batch_array(state,rows)
    assert np.array_equal(actual,expected),state
    assert np.array_equal(engine.batch_array(state,rows[::2]),expected[::2])
assert engine.batch_array('NOSYS',np.empty((0,7))).shape==(0,5)
for bad in (np.zeros(7),np.zeros((3,6))):
    try: engine.batch_array('NOSYS',bad)
    except ValueError: pass
    else: raise AssertionError('shape accepted')
jvt=Jvt(root/'payloads',None)
nnrows=np.column_stack([rows[:,0],rows[:,1],rows[:,1],data['sumpt'][:1024],data['jvfcorr'][:1024],data['haspv'][:1024]])
assert np.array_equal(jvt.nn.batch_array(nnrows),np.asarray(jvt.nn.batch(nnrows)))
erows=np.column_stack([rows[:,0],rows[:,1],jvt.nn.batch_array(nnrows)[:,1],data['ishs'][:1024]])
for sigma in (-1,0,1):
    assert np.array_equal(jvt.efficiency.batch_array(erows,sigma),np.asarray(jvt.efficiency.batch(erows.tolist(),sigma)))
assert jvt.nn.batch_array(np.empty((0,6))).shape==(0,2)
assert jvt.efficiency.batch_array(np.empty((0,4)),0).shape==(0,2)
print('117 states: bitwise legacy/array equality; strided inputs, empty batches, shapes and JVT PASS')
