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

"""Validate corrections and save a two-axis jet-pT histogram with uv run."""

import argparse
import json
from pathlib import Path
import sys

import awkward as ak
import hist
import numpy as np
from numpy.testing import assert_array_equal

from atlas_complete_corrections import JetCorrections, Jvt, Systematics, bundle_path
from atlas_complete_corrections import _atlas_corrections


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path("jet-pt-by-systematic.npz"))
    args = parser.parse_args()
    output = args.output.expanduser().resolve()
    if output.suffix != ".npz":
        parser.error("output must have a .npz suffix")
    if output.exists():
        parser.error(f"refusing to overwrite: {output}")
    if sys.version_info[:2] != (3, 14):
        raise RuntimeError("This wheel requires CPython 3.14")

    fixture = np.load(Path(__file__).with_name("reference-fixture.npz"))
    bundle = bundle_path()
    assert JetCorrections.load().bundle == bundle
    settings = json.loads((bundle / "nominal-manifest.json").read_text())["settings"]
    nominal = _atlas_corrections.Nominal(
        str(bundle / "nominal.json.gz"),
        settings["OffsetCorrection.DefaultMuRef"],
        settings["OffsetCorrection.DefaultNPVRef"],
        settings["MuScaleFactor"],
    )
    assert_array_equal(np.asarray(nominal.batch_array(fixture["rows"])), fixture["nominal"])

    p4 = fixture["p4"]
    variations = list(Systematics(bundle).variations(
        p4, fixture["mu"], fixture["npv"], fixture["bjet"]
    ))
    assert_array_equal(np.asarray([name for name, _ in variations]), fixture["names"])
    assert_array_equal(np.stack([value for _, value in variations]), fixture["variations"])

    jvt = Jvt(bundle, None)
    inputs = {name: fixture[name] for name in ("sumpt", "jvfcorr", "haspv", "ishs")}
    nominal_jvt = jvt.evaluate(p4, **inputs)
    for actual, name in zip(nominal_jvt, (
        "jvt_score", "jvt_selected", "jvt_weights", "jvt_valid"
    )):
        assert_array_equal(actual, fixture[name])
    assert_array_equal(jvt.evaluate(p4, **inputs, sigma=1)[2], fixture["jvt_up_weights"])
    assert_array_equal(jvt.evaluate(p4, **inputs, sigma=-1)[2], fixture["jvt_down_weights"])

    if "/cvmfs/" in Path("/proc/self/maps").read_text():
        raise RuntimeError("Runtime loaded a CVMFS library")

    categories = [name for name, _ in variations]
    jet_pt = hist.Hist(
        hist.axis.Regular(50, 0, 500, name="pt_gev", label="jet pT [GeV]"),
        hist.axis.StrCategory(categories, name="systematic", flow=False),
        storage=hist.storage.Int64(),
    )
    for name, values in variations:
        pt_gev = values[:, 0] / 1000.0
        if not np.all(np.isfinite(pt_gev)):
            raise RuntimeError(f"Nonfinite jet pT in {name}")
        jet_pt.fill(pt_gev=pt_gev, systematic=name)
    total_entries = int(jet_pt.sum(flow=True))
    if total_entries != len(p4) * len(categories):
        raise RuntimeError("Histogram entries do not match jets times systematics")
    flow_counts = jet_pt.values(flow=True)
    if flow_counts.shape != (52, len(categories)):
        raise RuntimeError(f"Unexpected histogram shape: {flow_counts.shape}")

    output.parent.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(
        output,
        axis_names=np.asarray(["pt_gev", "systematic"]),
        pt_edges_gev=jet_pt.axes[0].edges,
        systematic_categories=np.asarray(categories),
        counts=jet_pt.values(),
        underflow=flow_counts[0],
        overflow=flow_counts[-1],
        total_entries=np.int64(total_entries),
    )
    with np.load(output) as saved:
        if int(saved["total_entries"]) != total_entries:
            raise RuntimeError("Saved histogram entry count differs")
        assert_array_equal(saved["systematic_categories"], np.asarray(categories))

    print(f"Python {sys.version.split()[0]}")
    print(f"Validated {len(fixture['rows'])} jets, {len(variations)} systematic states, "
          "and 3 JVT settings")
    print("Histogram axes: pt_gev (50 bins, 0-500 GeV), systematic (category)")
    print(f"Total histogram entries (including underflow/overflow): {total_entries}")
    print(f"Saved histogram: {output}")
    print(f"Systematic categories ({len(categories)}):")
    for name in categories:
        print(f"  {name}")


if __name__ == "__main__":
    main()
