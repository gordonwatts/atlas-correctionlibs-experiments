# /// script
# requires-python = ">=3.14,<3.15"
# dependencies = [
#   "atlas-complete-corrections==0.1.0",
#   "numpy==2.5.4",
#   "awkward==2.14.0",
#   "awkward-cpp==57",
#   "fsspec==2026.9.0",
#   "packaging==26.3",
#   "hist==2.12.0",
#   "uproot==5.7.7",
# ]
# [tool.uv.sources]
# atlas-complete-corrections = { path = "./atlas_complete_corrections-0.1.0-cp314-cp314-linux_x86_64.whl" }
# ///

"""Correct every jet in an MC PHYSLITE ROOT file and save a pT histogram.

Reads CollectionTree directly with uproot, in 1000-event chunks. The output
<input-filename>.jet-pt-by-systematic.npz is written in the current directory.
"""

import argparse
from pathlib import Path
import sys

import awkward as ak
import hist
import numpy as np
import uproot

from atlas_complete_corrections import JetCorrections, Jvt, Systematics, bundle_path


JET_FIELDS = ["JetConstitScaleMomentum_" + name for name in ("pt", "eta", "phi", "m")]
JET_FIELDS += [
    "DetectorEta", "ActiveArea4vec_pt", "SumPtChargedPFOPt500",
    "EnergyPerSampling", "NumTrkPt1000", "TrackWidthPt1000",
]
EVENT_FIELDS = {
    "vertex_type": "PrimaryVerticesAuxDyn.vertexType",
    "rho": "Kt4EMPFlowNeutEventShapeAuxDyn.Density",
    "mu": "EventInfoAuxDyn.averageInteractionsPerCrossing",
    "event_number": "EventInfoAuxDyn.eventNumber",
}
JET_PREFIX = "AnalysisJetsAuxDyn."
TRUTH_PREFIX = "AntiKt4TruthDressedWZJetsAuxDyn."
EXTRA_BRANCHES = [
    "PrimaryVerticesAuxDyn.trackParticleLinks",
    JET_PREFIX + "PartonTruthLabelID",
    JET_PREFIX + "SumPtTrkPt500",
    JET_PREFIX + "JVFCorr",
    *(JET_PREFIX + name for name in ("eta", "phi")),
    *(TRUTH_PREFIX + name for name in ("pt", "eta", "phi")),
]
BRANCHES = [*(JET_PREFIX + name for name in JET_FIELDS),
            *EVENT_FIELDS.values(), *EXTRA_BRANCHES]


def flatten(values):
    return ak.to_numpy(ak.flatten(values, axis=1))


def read_chunk(columns, nominal):
    jets = {name: columns[JET_PREFIX + name] for name in JET_FIELDS}
    events = {name: columns[branch] for name, branch in EVENT_FIELDS.items()}
    result = nominal.evaluate(jets, events)
    rows = result["inputs"]
    counts = result["counts"]
    stages = result["stages"]
    p4 = stages[:, 8:12]
    if p4.shape != (len(rows), 4) or not np.all(np.isfinite(p4)):
        raise RuntimeError("Nominal correction produced invalid jet kinematics")

    vertex_types = events["vertex_type"]
    primary_vertex = ak.fill_none(
        ak.firsts(ak.local_index(vertex_types)[vertex_types == 1]), 0
    )
    jet_vertex = np.repeat(ak.to_numpy(primary_vertex).astype(np.int64), counts)
    tracks = ak.flatten(columns[JET_PREFIX + "SumPtTrkPt500"], axis=1)
    if bool(ak.any(ak.num(tracks, axis=1) <= jet_vertex)):
        raise ValueError("Missing per-vertex SumPtTrkPt500 for a jet")
    sumpt = ak.to_numpy(ak.firsts(
        tracks[ak.local_index(tracks, axis=1) == jet_vertex[:, None]]
    )).astype(np.float64)
    haspv = np.repeat(ak.to_numpy(ak.any(vertex_types == 1, axis=1)), counts)
    links = columns["PrimaryVerticesAuxDyn.trackParticleLinks"]
    npv = np.repeat(
        ak.to_numpy(ak.sum(ak.num(links, axis=2) > 1, axis=1)).astype(np.float64),
        counts,
    )
    mu = np.repeat(ak.to_numpy(events["mu"]).astype(np.float64), counts)
    bjet = flatten(columns[JET_PREFIX + "PartonTruthLabelID"]) == 5
    jvfcorr = flatten(columns[JET_PREFIX + "JVFCorr"]).astype(np.float64)

    reco = ak.zip({
        name: ak.values_astype(columns[JET_PREFIX + name], np.float64)
        for name in ("eta", "phi")
    })
    truth = ak.zip({
        name: ak.values_astype(columns[TRUTH_PREFIX + name], np.float64)
        for name in ("pt", "eta", "phi")
    })
    pairs = ak.cartesian({"reco": reco, "truth": truth}, axis=1, nested=True)
    dphi = (pairs.reco.phi - pairs.truth.phi + np.pi) % (2 * np.pi) - np.pi
    dr = ak.values_astype(
        np.sqrt((pairs.reco.eta - pairs.truth.eta) ** 2 + dphi ** 2), np.float32
    )
    ishs = flatten(ak.any(
        (dr < np.float32(0.3)) & (pairs.truth.pt > 10000), axis=2
    ))
    inputs = {"sumpt": sumpt, "jvfcorr": jvfcorr, "haspv": haspv, "ishs": ishs}
    for name, values in {"mu": mu, "npv": npv, "bjet": bjet, **inputs}.items():
        if values.shape != (len(p4),):
            raise RuntimeError(f"Unexpected {name} shape: {values.shape}")
    return p4, mu, npv, bjet, inputs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="MC PHYSLITE ROOT file")
    args = parser.parse_args()
    input_name = args.input.name
    source = args.input.expanduser().resolve()
    if not source.is_file():
        parser.error(f"input file does not exist: {source}")
    output = Path.cwd() / (input_name + ".jet-pt-by-systematic.npz")
    if output.exists():
        parser.error(f"refusing to overwrite: {output}")
    if sys.version_info[:2] != (3, 14):
        raise RuntimeError("This wheel requires CPython 3.14")

    nominal = JetCorrections.load()
    bundle = bundle_path()
    if nominal.bundle != bundle:
        raise RuntimeError("Nominal correction did not load the wheel payload")
    systematics = Systematics(bundle)
    jvt = Jvt(bundle, None)
    kinematic_names = list(systematics.engine.names())
    efficiency_names = ["JET_NNJvtEfficiency__1up", "JET_NNJvtEfficiency__1down"]
    categories = kinematic_names + efficiency_names
    if len(set(categories)) != len(categories):
        raise RuntimeError("Duplicate systematic categories")
    category_index = {name: index for index, name in enumerate(categories)}
    jet_pt = hist.Hist(
        hist.axis.Regular(50, 0, 500, name="pt_gev", label="jet pT [GeV]"),
        hist.axis.StrCategory(categories, name="systematic", flow=False),
        storage=hist.storage.Int64(),
    )
    selected_counts = np.zeros(len(categories), dtype=np.int64)
    valid_counts = np.zeros(len(categories), dtype=np.int64)
    weight_sums = np.zeros(len(categories), dtype=np.float64)

    def record(name, varied_p4, inputs, sigma=0):
        pt_gev = varied_p4[:, 0] / 1000.0
        if not np.all(np.isfinite(pt_gev)):
            raise RuntimeError(f"Nonfinite jet pT in {name}")
        _, selected, weights, valid = jvt.evaluate(varied_p4, **inputs, sigma=sigma)
        effective_weights = np.where(valid, weights, 1.0)
        if not np.all(np.isfinite(effective_weights)):
            raise RuntimeError(f"Nonfinite valid JVT weights in {name}")
        jet_pt.fill(pt_gev=pt_gev, systematic=name)
        index = category_index[name]
        selected_counts[index] += np.count_nonzero(selected)
        valid_counts[index] += np.count_nonzero(valid)
        weight_sums[index] += np.sum(effective_weights, dtype=np.float64)

    events_seen = 0
    jets_seen = 0
    with uproot.open(source) as root_file:
        if "CollectionTree" not in root_file:
            parser.error("input ROOT file has no CollectionTree")
        tree = root_file["CollectionTree"]
        missing = sorted(set(BRANCHES) - set(tree.keys()))
        if missing:
            parser.error("missing PHYSLITE branches: " + ", ".join(missing))
        for columns in tree.iterate(BRANCHES, step_size=1000, library="ak", how=dict):
            events_seen += len(columns[EVENT_FIELDS["mu"]])
            p4, mu, npv, bjet, inputs = read_chunk(columns, nominal)
            jets_seen += len(p4)
            if len(p4):
                observed = []
                for name, varied_p4 in systematics.variations(p4, mu, npv, bjet):
                    observed.append(name)
                    record(name, varied_p4, inputs)
                if observed != kinematic_names:
                    raise RuntimeError("Systematic names differ from the wheel names")
                for sigma, name in ((1, efficiency_names[0]), (-1, efficiency_names[1])):
                    record(name, p4, inputs, sigma=sigma)
            print(f"Processed {events_seen}/{tree.num_entries} events, {jets_seen} jets", flush=True)

    total_entries = int(jet_pt.sum(flow=True))
    if total_entries != jets_seen * len(categories):
        raise RuntimeError("Histogram entries do not match jets times systematics")
    flow_counts = jet_pt.values(flow=True)
    if flow_counts.shape != (52, len(categories)):
        raise RuntimeError(f"Unexpected histogram shape: {flow_counts.shape}")
    np.savez_compressed(
        output,
        axis_names=np.asarray(["pt_gev", "systematic"]),
        pt_edges_gev=jet_pt.axes[0].edges,
        systematic_categories=np.asarray(categories),
        counts=jet_pt.values(),
        underflow=flow_counts[0],
        overflow=flow_counts[-1],
        jvt_selected_counts=selected_counts,
        jvt_valid_counts=valid_counts,
        jvt_weight_sums=weight_sums,
        events=np.int64(events_seen),
        jets=np.int64(jets_seen),
        total_entries=np.int64(total_entries),
    )
    print(f"Python {sys.version.split()[0]}")
    print(f"Processed {events_seen} events and {jets_seen} jets through nominal calibration, "
          f"{len(kinematic_names)} kinematic states, and 2 JVT efficiency variations")
    print("Histogram axes: pt_gev (50 bins, 0-500 GeV), systematic (category)")
    print(f"Total histogram entries (including underflow/overflow): {total_entries}")
    print(f"Saved histogram: {output}")
    print(f"Systematic categories ({len(categories)}):")
    for name in categories:
        print(f"  {name}")


if __name__ == "__main__":
    main()
