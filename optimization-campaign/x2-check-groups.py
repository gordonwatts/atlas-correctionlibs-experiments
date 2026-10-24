from pathlib import Path
import numpy as np
from _atlas_corrections import JetSystematics
r=Path('/home/gwatts/atlas/correctionlib_tests')
d=np.load(r/'results/jvt-inputs.npz');n=512
rows=np.column_stack([d['p4'][:n],d['mu'][:n],d['npv'][:n],d['bjet'][:n]])
engine=JetSystematics(str(r/'payloads/uncertainty-tables.json.gz'),str(r/'payloads/uncertainty-manifest.json'))
assert [name for group in engine.groups() for name in group]==engine.names()
for group in engine.groups():
 actual=engine.batch_group_array(group,rows)
 assert actual.shape==(len(group),n,5)
 for name,values in zip(group,actual):assert np.array_equal(values,engine.batch_array(name,rows)),name
 assert engine.batch_group_array(group,np.empty((0,7))).shape==(len(group),0,5)
 assert np.array_equal(engine.batch_group_array(group,rows[::2]),actual[:,::2])
for names,values in [([],rows),(['NOSYS',engine.names()[1]],rows),([engine.names()[1],engine.names()[-1]],rows),(['NOSYS'],np.zeros((1,6)))]:
 try:engine.batch_group_array(names,values)
 except (ValueError,IndexError):pass
 else:raise AssertionError((names,values.shape))
print('PASS: grouped/scalar equality for all 117 states; empty, strided and invalid groups/shapes checked')
