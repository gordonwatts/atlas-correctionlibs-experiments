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
# ]
# [tool.uv.sources]
# atlas-complete-corrections = { path = "./atlas_complete_corrections-0.1.0-cp314-cp314-linux_x86_64.whl" }
# ///

"""Run all bundled jet corrections on an input .npz and save a pT histogram.

Input arrays: rows (N, 14), bjet, sumpt, jvfcorr, haspv, ishs (each N).
The rows columns are input pt, eta, phi, mass, detector eta, active-area pt,
rho, mu, NPV, charged fraction, Tile fraction, EM3 fraction, number of tracks,
and track width. Momentum and energy inputs are in MeV; output pT is in GeV.
"""

import argparse
import json
from pathlib import Path
import sys
from zipfile import BadZipFile

import hist
import numpy as np

from atlas_complete_corrections import Jvt, Systematics, bundle_path
from atlas_complete_corrections import _atlas_corrections


def load_input(path, parser):
    if path.suffix.lower() != ".npz" or not path.is_file():
        parser.error("input must be an existing .npz file")
    try:
        with np.load(path, allow_pickle=False) as data:
            required = ("rows", "bjet", "sumpt", "jvfcorr", "haspv", "ishs")
            missing = [name for name in required if name not in data.files]
            if missing:
                parser.error("missing input arrays: " + ", ".join(missing))
            rows = np.asarray(data["rows"], dtype=np.float64)
            if rows.ndim != 2 or rows.shape[1] != 14 or len(rows) == 0:
                parser.error("rows must have shape (N, 14) with N > 0")
            if not np.all(np.isfinite(rows)):
                parser.error("rows contains nonfinite values")
            inputs = {}
            for name in required[1:]:
                values = np.asarray(data[name])
                if values.shape != (len(rows),):
                    parser.error(f"{name} must have shape ({len(rows)},)")
                if name in ("bjet", "haspv", "ishs"):
                    if not np.all((values == 0) | (values == 1)):
                        parser.error(f"{name} must contain only booleans or 0/1")
                    inputs[name] = values.astype(np.bool_)
                else:
                    inputs[name] = values.astype(np.float64)
                    if not np.all(np.isfinite(inputs[name])):
                        parser.error(f"{name} contains nonfinite values")
    except (OSError, ValueError, TypeError, BadZipFile) as error:
        parser.error(f"cannot read input: {error}")
    return rows, inputs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="jet input .npz file")
    args = parser.parse_args()
    source = args.input.expanduser().resolve()
    rows, inputs = load_input(source, parser)
    output = source.with_name(source.stem + "-jet-pt-by-systematic.npz")
    if output.exists():
        parser.error(f"refusing to overwrite: {output}")
    if sys.version_info[:2] != (3, 14):
        raise RuntimeError("This wheel requires CPython 3.14")

    bundle = bundle_path()
    settings = json.loads((bundle / "nominal-manifest.json").read_text())["settings"]
    nominal = _atlas_corrections.Nominal(
        str(bundle / "nominal.json.gz"),
        settings["OffsetCorrection.DefaultMuRef"],
        settings["OffsetCorrection.DefaultNPVRef"],
        settings["MuScaleFactor"],
    )
    stages = np.asarray(nominal.batch_array(rows), dtype=np.float64).reshape((-1, 12))
    if stages.shape != (len(rows), 12):
        raise RuntimeError(f"Unexpected nominal correction shape: {stages.shape}")
    if not np.all(np.isfinite(stages)):
        raise RuntimeError("Nominal correction produced nonfinite jet kinematics")
    p4 = stages[:, 8:12]

    systematics = Systematics(bundle)
    kinematic_names = list(systematics.engine.names())
    efficiency_names = ["JET_NNJvtEfficiency__1up", "JET_NNJvtEfficiency__1down"]
    categories = kinematic_names + efficiency_names
    if len(set(categories)) != len(categories):
        raise RuntimeError("Duplicate systematic categories")
    jet_pt = hist.Hist(
        hist.axis.Regular(50, 0, 500, name="pt_gev", label="jet pT [GeV]"),
        hist.axis.StrCategory(categories, name="systematic", flow=False),
        storage=hist.storage.Int64(),
    )

    jvt = Jvt(bundle, None)
    jvt_inputs = {name: inputs[name] for name in ("sumpt", "jvfcorr", "haspv", "ishs")}
    selected_counts = []
    valid_counts = []
    weight_sums = []

    def record(name, varied_p4, sigma=0):
        pt_gev = varied_p4[:, 0] / 1000.0
        if not np.all(np.isfinite(pt_gev)):
            raise RuntimeError(f"Nonfinite jet pT in {name}")
        _, selected, weights, valid = jvt.evaluate(varied_p4, **jvt_inputs, sigma=sigma)
        effective_weights = np.where(valid, weights, 1.0)
        if not np.all(np.isfinite(effective_weights)):
            raise RuntimeError(f"Nonfinite valid JVT weights in {name}")
        jet_pt.fill(pt_gev=pt_gev, systematic=name)
        selected_counts.append(np.count_nonzero(selected))
        valid_counts.append(np.count_nonzero(valid))
        weight_sums.append(np.sum(effective_weights, dtype=np.float64))

    observed = []
    for name, varied_p4 in systematics.variations(
        p4, rows[:, 7], rows[:, 8], inputs["bjet"]
    ):
        observed.append(name)
        record(name, varied_p4)
    if observed != kinematic_names:
        raise RuntimeError("Systematic names differ from the wheel's declared names")
    for sigma, name in ((1, efficiency_names[0]), (-1, efficiency_names[1])):
        record(name, p4, sigma=sigma)

    total_entries = int(jet_pt.sum(flow=True))
    if total_entries != len(rows) * len(categories):
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
        jvt_selected_counts=np.asarray(selected_counts, dtype=np.int64),
        jvt_valid_counts=np.asarray(valid_counts, dtype=np.int64),
        jvt_weight_sums=np.asarray(weight_sums, dtype=np.float64),
        total_entries=np.int64(total_entries),
    )

    print(f"Python {sys.version.split()[0]}")
    print(f"Processed {len(rows)} jets through nominal calibration, "
          f"{len(kinematic_names)} kinematic states, and 2 JVT efficiency variations")
    print("Histogram axes: pt_gev (50 bins, 0-500 GeV), systematic (category)")
    print(f"Total histogram entries (including underflow/overflow): {total_entries}")
    print(f"Saved histogram: {output}")
    print(f"Systematic categories ({len(categories)}):")
    for name in categories:
        print(f"  {name}")


if __name__ == "__main__":
    main()
