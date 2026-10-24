from pathlib import Path
import awkward as ak
import numpy as np
import uproot,json
root=Path('/home/gwatts/atlas/correctionlib_tests/optimization-campaign')
before=root/'09-single-thread/runs/C-2/results'
after=root/'11-cached-gaussian/runs/C-2/results'
report={}
for filename in ('columnar-kinematics.root','columnar-jvt.root'):
    states=0;fields=0
    with uproot.open(before/filename) as old,uproot.open(after/filename) as new:
        assert old.keys()==new.keys(),filename
        for name in old.keys(cycle=False):
            if name=='__kinematic_cache__':continue  # Backend hash intentionally changes.
            lhs=old[name].arrays();rhs=new[name].arrays()
            lf,ll,lb=ak.to_buffers(lhs);rf,rl,rb=ak.to_buffers(rhs)
            assert lf==rf and ll==rl and lb.keys()==rb.keys(),(filename,name)
            for field in lb:
                assert np.array_equal(lb[field],rb[field]),(filename,name,field)
            states+=1;fields+=len(lhs.fields)
    report[filename]=dict(states=states,fields=fields,bitwise_equal=True)
path=root/'checks/full-smearing-parity.json'
path.write_text(json.dumps(report,indent=2));print(path.read_text())
