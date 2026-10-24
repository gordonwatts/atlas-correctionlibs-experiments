from pathlib import Path
import json, sys
import awkward as ak
import numpy as np
import uproot
root=Path('/home/gwatts/atlas/correctionlib_tests')
sys.path[:0]=[str(root/'python'),str(root/'install/python')]
from atlas_correctionlib.kinematic_cache import read
source=root/'optimization-campaign/04-reuse-kinematics/runs/C-1/results/columnar-kinematics.root'
with uproot.open(source) as f:
    meta=json.loads(str(f['__kinematic_cache__']))
    a=f['NOSYS'].arrays(entry_stop=2,how=dict)
counts=ak.to_numpy(ak.num(a['pt']))
events=ak.to_numpy(a['eventNumber'])
dest=root/'optimization-campaign/checks/cache-check.root'
def write(data):
    with uproot.recreate(dest) as f:
        f['__kinematic_cache__']=json.dumps(dict(version=1,fingerprint=meta['fingerprint'],names=['NOSYS']))
        f['NOSYS']=data
def reject(signature,names):
    try: next(read(dest,signature,names,counts,events))
    except ValueError: return
    raise AssertionError('Invalid cache accepted')
write(a)
assert next(read(dest,meta['fingerprint'],['NOSYS'],counts,events))[0]=='NOSYS'
reject('changed-inputs-or-payload',['NOSYS'])
reject(meta['fingerprint'],['NOSYS','unknown'])
write(dict(a,eventNumber=a['eventNumber']+1));reject(meta['fingerprint'],['NOSYS'])
write(dict(a,originalJetIndex=a['originalJetIndex']+1));reject(meta['fingerprint'],['NOSYS'])
write(dict(a,status=ak.zeros_like(a['status'])));reject(meta['fingerprint'],['NOSYS'])
print('Cache provenance, state set, event identity, jet ordering and correction-status rejection PASS')
