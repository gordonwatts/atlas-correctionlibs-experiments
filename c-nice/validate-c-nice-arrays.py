from pathlib import Path
import json
import awkward as ak
import numpy as np
import uproot
from atlas_jet_corrections import JetCorrections, FIELDS, EVENT_BRANCHES
root=Path('/home/gwatts/atlas/correctionlib_tests')
old=np.load(root/'c-nice/results/C-4/results/columnar-nominal.npz')
old_inputs=np.load(root/'c-nice/results/C-4/results/columnar-inputs.npz')['rows']
engine=JetCorrections.load(root/'payloads')
file=Path('/cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt/src/AsgExternal/Asg_Test/cmt/TestFileLiteR3MC.txt').read_text().strip()
offset=0; events=0; input_differences=0; output_differences=0; max_delta=0.
with uproot.open(file) as f:
    for a in f['CollectionTree'].iterate(expressions=['AnalysisJetsAuxDyn.'+x for x in FIELDS]+list(EVENT_BRANCHES.values()),step_size=1000,how=dict):
        result=engine.evaluate({x:a['AnalysisJetsAuxDyn.'+x] for x in FIELDS},{x:a[y] for x,y in EVENT_BRANCHES.items()})
        n=len(result['stages']); stop=offset+n
        assert np.array_equal(result['counts'],old['counts'][events:events+len(result['counts'])])
        input_differences+=int(np.count_nonzero(result['inputs']!=old_inputs[offset:stop]))
        output_differences+=int(np.count_nonzero(result['stages']!=old['values'][offset:stop]))
        delta=np.abs(result['stages']-old['values'][offset:stop])
        max_delta=max(max_delta,float(np.max(delta,initial=0)))
        assert np.all(delta<=0.01+2e-6*np.abs(old['values'][offset:stop]))
        offset=stop; events+=len(result['counts'])
assert offset==196606 and events==20000
report=dict(events=events,jets=offset,input_differences=input_differences,output_differences=output_differences,max_absolute_delta=max_delta,passed=True)
(root/'c-nice/array-validation.json').write_text(json.dumps(report,indent=2))
print(report)
