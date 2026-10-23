import json
from pathlib import Path
import awkward as ak
import hist
import matplotlib.pyplot as plt
import numpy as np
import uproot

h = hist.Hist.new.Reg(50, 0, 500, name='pt', label='Jet $p_T$ [GeV]').Double()
events = jets = 0
with uproot.open('output.root') as f:
    for a in f['analysis'].iterate(expressions=['stored_pt_NOSYS','stored_eta_NOSYS'], step_size='50 MB', library='ak', how=dict):
        pt = a['stored_pt_NOSYS']/1000.
        eta = a['stored_eta_NOSYS']
        keep = (pt > 20.) & (abs(eta) < 2.5)
        h.fill(pt=ak.to_numpy(ak.flatten(pt[keep],axis=1)))
        events += len(pt)
        jets += int(ak.sum(ak.num(pt,axis=1)))
assert events == 20000 and jets == 196606
fig, ax = plt.subplots(figsize=(6,4))
h.plot1d(ax=ax,histtype='step',label='Stored PHYSLITE jets via A')
ax.set_ylabel('Jets / 10 GeV')
ax.legend()
fig.tight_layout()
fig.savefig('a-stored-jet-pt.png',dpi=150)
plt.close(fig)
Path('histogram.json').write_text(json.dumps(dict(events=events,jets=jets,counts_including_flow=h.values(flow=True).tolist()),indent=2))
