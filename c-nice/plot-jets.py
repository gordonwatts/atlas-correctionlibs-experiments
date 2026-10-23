import awkward as ak
import hist
import matplotlib.pyplot as plt
import uproot

from atlas_jet_corrections import JetCorrections  # Proposed package.

# Load once, outside the event-chunk loop. Pin a real local bundle.
jets = JetCorrections.load("/home/gwatts/atlas/correctionlib_tests/payloads")

# Explicit bindings make the source columns visible. The bundle should
# eventually supply an inspectable, validated PHYSLITE input schema.
jet_fields = [
    "JetConstitScaleMomentum_pt", "JetConstitScaleMomentum_eta",
    "JetConstitScaleMomentum_phi", "JetConstitScaleMomentum_m",
    "DetectorEta", "ActiveArea4vec_pt", "SumPtChargedPFOPt500",
    "EnergyPerSampling", "NumTrkPt1000", "TrackWidthPt1000",
]
event_branches = {
    "vertex_type": "PrimaryVerticesAuxDyn.vertexType",
    "rho": "Kt4EMPFlowNeutEventShapeAuxDyn.Density",
    "mu": "EventInfoAuxDyn.averageInteractionsPerCrossing",
    "event_number": "EventInfoAuxDyn.eventNumber",
}
jet_branches = {name: "AnalysisJetsAuxDyn." + name for name in jet_fields}
branches = list(jet_branches.values()) + list(event_branches.values())

h_pt = hist.Hist.new.Reg(50, 0, 500, name="pt", label="Jet $p_T$ [GeV]").Double()

with uproot.open("/cvmfs/atlas-nightlies.cern.ch/repo/data/data-art/ASG/DAOD_PHYSLITE/p7266/mc23_13p6TeV.601229.PhPy8EG_A14_ttbar_hdamp258p75_SingleLep.deriv.DAOD_PHYSLITE.e8514_e8528_s4162_s4114_r15540_r15516_p7266/DAOD_PHYSLITE.49568713._001693.pool.root.1") as source:
    tree = source["CollectionTree"]
    for columns in tree.iterate(
        expressions=branches, step_size="50 MB", library="ak", how=dict,
    ):
        result = jets.evaluate(
            {name: columns[branch] for name, branch in jet_branches.items()},
            {name: columns[branch] for name, branch in event_branches.items()},
            systematics=["NOSYS"],
            outputs=["calibration"],  # Proposed stage selection.
        )
        nominal = result["NOSYS"]  # Proposed named result fields.

        # Make the invalid-jet policy explicit for this example.
        if not bool(ak.all(nominal.valid, axis=None)):
            raise RuntimeError("Calibration returned invalid jets; inspect statuses")

        pt_gev = nominal.pt / 1000.0
        eta = columns[jet_branches["JetConstitScaleMomentum_eta"]]
        selected = (pt_gev > 20.0) & (abs(eta) < 2.5)
        h_pt.fill(pt=ak.to_numpy(ak.flatten(pt_gev[selected], axis=1)))

fig, ax = plt.subplots(figsize=(6, 4))
h_pt.plot1d(ax=ax, histtype="step", label="Nominal calibrated jets")
ax.set_ylabel("Jets / 10 GeV")
ax.legend()
fig.tight_layout()
fig.savefig("jet-pt.png", dpi=150)
plt.close(fig)
