from pathlib import Path
import sys,json,uproot,awkward as ak,numpy as np
r=Path('/home/gwatts/atlas/correctionlib_tests/optimization-campaign')
old,new=sys.argv[1:3]
before=r/old/'runs/C-2/results';after=r/new/'runs/C-2/results'
def identical(a,b):
 return a.shape==b.shape and a.dtype==b.dtype and a.tobytes()==b.tobytes()
report={}
for filename in ('columnar-nominal.npz','columnar-inputs.npz','jvt-inputs.npz'):
 with np.load(before/filename) as a,np.load(after/filename) as b:
  assert set(a.files)==set(b.files)
  for key in a.files:assert identical(a[key],b[key]),(filename,key)
 report[filename]={'bitwise_equal':True}
for filename in ('columnar-kinematics.root','columnar-jvt.root'):
 states=0
 with uproot.open(before/filename) as a,uproot.open(after/filename) as b:
  assert a.keys()==b.keys()
  for name in a.keys(cycle=False):
   if name=='__kinematic_cache__':continue
   af,al,ab=ak.to_buffers(a[name].arrays());bf,bl,bb=ak.to_buffers(b[name].arrays())
   assert af==bf and al==bl and ab.keys()==bb.keys(),name
   for key in ab:assert identical(ab[key],bb[key]),(filename,name,key)
   states+=1
 report[filename]={'states':states,'bitwise_equal':True}
report.update(before=old,after=new)
(r/'x2-checks'/f'{old}-to-{new}-parity.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report),flush=True)


