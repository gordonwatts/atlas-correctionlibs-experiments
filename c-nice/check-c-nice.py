from pathlib import Path
import json
import awkward as ak
import numpy as np
import uproot
from atlas_jet_corrections import JetCorrections, FIELDS, EVENT_BRANCHES
root=Path('/home/gwatts/atlas/correctionlib_tests')
file=Path('/cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt/src/AsgExternal/Asg_Test/cmt/TestFileLiteR3MC.txt').read_text().strip()
engine=JetCorrections.load(root/'payloads')
with uproot.open(file) as f:
    a=f['CollectionTree'].arrays(['AnalysisJetsAuxDyn.'+x for x in FIELDS]+list(EVENT_BRANCHES.values()),entry_stop=30,how=dict)
j={x:a['AnalysisJetsAuxDyn.'+x] for x in FIELDS}
e={x:a[y] for x,y in EVENT_BRANCHES.items()}
whole=engine.evaluate(j,e)
parts=[engine.evaluate({k:v[i:i+7] for k,v in j.items()},{k:v[i:i+7] for k,v in e.items()})['stages'] for i in range(0,30,7)]
assert np.array_equal(whole['stages'],np.concatenate(parts))
empty=engine.evaluate({k:v[:0] for k,v in j.items()},{k:v[:0] for k,v in e.items()})
assert empty['stages'].shape==(0,12)
zero=engine.evaluate({k:v[:1,:0] for k,v in j.items()},{k:v[:1] for k,v in e.items()})
assert ak.to_list(zero['NOSYS'].pt)==[[]]
for badj,bade,kwargs in [(dict(j,DetectorEta=j['DetectorEta'][:2]),e,{}),({k:v for k,v in j.items() if k!='DetectorEta'},e,{}),(j,e,{'systematics':['recommended']})]:
    try: engine.evaluate(badj,bade,**kwargs)
    except ValueError: pass
    else: raise AssertionError('Expected input rejection')
report={'chunk_invariance':'bitwise equal','empty_chunk':'pass','zero_jet_event':'pass','shape_missing_and_unsupported_inputs':'pass'}
(root/'c-nice/interface-checks.json').write_text(json.dumps(report,indent=2))
print(report)
