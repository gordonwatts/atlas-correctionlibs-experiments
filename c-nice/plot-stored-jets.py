"""Read and plot stored PHYSLITE jet momenta; no correction backend imported."""
import argparse
import json
from pathlib import Path

import awkward as ak
import hist
import matplotlib.pyplot as plt
import numpy as np
import uproot

parser = argparse.ArgumentParser()
parser.add_argument('--read-calibration-inputs', action='store_true')
args = parser.parse_args()
sample = Path('/cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt/src/AsgExternal/Asg_Test/cmt/TestFileLiteR3MC.txt').read_text().strip()
branches = ['AnalysisJetsAuxDyn.pt', 'AnalysisJetsAuxDyn.eta']
if args.read_calibration_inputs:
    # I/O control: read the original C-nice columns as well, without using them.
    fields = ['JetConstitScaleMomentum_' + x for x in ('pt', 'eta', 'phi', 'm')]
    fields += ['DetectorEta', 'ActiveArea4vec_pt', 'SumPtChargedPFOPt500',
               'EnergyPerSampling', 'NumTrkPt1000', 'TrackWidthPt1000']
    branches += ['AnalysisJetsAuxDyn.' + x for x in fields]
    branches += ['PrimaryVerticesAuxDyn.vertexType',
                 'Kt4EMPFlowNeutEventShapeAuxDyn.Density',
                 'EventInfoAuxDyn.averageInteractionsPerCrossing',
                 'EventInfoAuxDyn.eventNumber']

h_pt = hist.Hist.new.Reg(50, 0, 500, name='pt', label='Jet $p_T$ [GeV]').Double()
events = jets = selected_jets = 0
with uproot.open(sample) as source:
    for columns in source['CollectionTree'].iterate(
        expressions=branches, step_size='50 MB', library='ak', how=dict,
    ):
        pt_gev = columns['AnalysisJetsAuxDyn.pt'] / 1000.0
        eta = columns['AnalysisJetsAuxDyn.eta']
        if not bool(ak.all(np.isfinite(pt_gev) & np.isfinite(eta), axis=None)):
            raise ValueError('Nonfinite stored jet kinematics')
        selected = (pt_gev > 20.0) & (abs(eta) < 2.5)
        h_pt.fill(pt=ak.to_numpy(ak.flatten(pt_gev[selected], axis=1)))
        events += len(pt_gev)
        jets += int(ak.sum(ak.num(pt_gev, axis=1)))
        selected_jets += int(ak.sum(selected))

assert events == 20000 and jets == 196606
assert int(h_pt.sum(flow=True)) == selected_jets
fig, ax = plt.subplots(figsize=(6, 4))
h_pt.plot1d(ax=ax, histtype='step', label='Stored PHYSLITE jets')
ax.set_ylabel('Jets / 10 GeV')
ax.legend()
fig.tight_layout()
fig.savefig('stored-jet-pt.png', dpi=150)
plt.close(fig)
Path('histogram.json').write_text(json.dumps(dict(
    events=events, jets=jets, selected_jets=selected_jets,
    counts_including_flow=h_pt.values(flow=True).tolist(),
    branches=branches, read_calibration_inputs=args.read_calibration_inputs,
), indent=2))
