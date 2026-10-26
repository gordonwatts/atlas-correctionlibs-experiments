#!/usr/bin/env python3
"""Numerically check every wheel correction stage without release setup."""
import argparse
import json
import os
from pathlib import Path
import sys

import awkward as ak
import numpy as np
from numpy.testing import assert_array_equal
import atlas_complete_corrections as acc
from atlas_complete_corrections import JetCorrections, Jvt, Systematics
from atlas_complete_corrections import _atlas_corrections


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--fixture", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    if os.environ.get("ATLAS_LOCAL_ROOT_BASE") or os.environ.get("AtlasVersion"):
        raise RuntimeError("ATLAS release variables are still set")
    if os.environ.get("LD_LIBRARY_PATH") or os.environ.get("PYTHONPATH"):
        raise RuntimeError("Library/Python path override is still set")
    assert str(Path(acc.__file__).resolve()).startswith(sys.prefix)
    assert str(Path(_atlas_corrections.__file__).resolve()).startswith(sys.prefix)
    assert all((acc.bundle_path() / name).is_file() for name in (
        "nominal.json.gz", "uncertainty-tables.json.gz", "nnjvt-network.json"))

    data = np.load(args.fixture)
    bundle = acc.bundle_path()
    settings = json.loads((bundle / "nominal-manifest.json").read_text())["settings"]
    nominal = _atlas_corrections.Nominal(str(bundle / "nominal.json.gz"),
        settings["OffsetCorrection.DefaultMuRef"], settings["OffsetCorrection.DefaultNPVRef"],
        settings["MuScaleFactor"])
    assert_array_equal(np.asarray(nominal.batch_array(data["rows"])), data["nominal"])

    engine = Systematics(bundle)
    variations = list(engine.variations(data["p4"], data["mu"], data["npv"], data["bjet"]))
    assert_array_equal(np.asarray([name for name, _ in variations]), data["names"])
    assert_array_equal(np.stack([value for _, value in variations]), data["variations"])

    jvt = Jvt(bundle, None)
    jvt_inputs = {name: data[name] for name in ("sumpt", "jvfcorr", "haspv", "ishs")}
    score, selected, weights, valid = jvt.evaluate(data["p4"], **jvt_inputs)
    for actual, name in zip((score, selected, weights, valid),
                            ("jvt_score", "jvt_selected", "jvt_weights", "jvt_valid")):
        assert_array_equal(actual, data[name])
    assert_array_equal(jvt.evaluate(data["p4"], **jvt_inputs, sigma=1)[2], data["jvt_up_weights"])
    assert_array_equal(jvt.evaluate(data["p4"], **jvt_inputs, sigma=-1)[2], data["jvt_down_weights"])

    facade = JetCorrections.load()
    jet = {
        "JetConstitScaleMomentum_pt": ak.Array([[50000.]]),
        "JetConstitScaleMomentum_eta": ak.Array([[0.2]]),
        "JetConstitScaleMomentum_phi": ak.Array([[0.1]]),
        "JetConstitScaleMomentum_m": ak.Array([[5000.]]),
        "DetectorEta": ak.Array([[0.2]]),
        "ActiveArea4vec_pt": ak.Array([[1000.]]),
        "SumPtChargedPFOPt500": ak.Array([[[10000.]]]),
        "EnergyPerSampling": ak.Array([[[100.] * 19]]),
        "NumTrkPt1000": ak.Array([[[4.]]]),
        "TrackWidthPt1000": ak.Array([[[0.1]]]),
    }
    event = {"vertex_type": ak.Array([[1]]), "rho": ak.Array([1000.]),
             "mu": ak.Array([30.]), "event_number": ak.Array([1])}
    result = facade.evaluate(jet, event)
    assert bool(ak.all(result["NOSYS"].valid))
    assert str(facade.bundle) == str(bundle)

    mapped = Path("/proc/self/maps").read_text()
    if "/cvmfs/" in mapped:
        raise RuntimeError("Runtime still loaded CVMFS libraries")
    if str(Path(acc.__file__).resolve().parent / "libs/libstdc++.so.6") not in mapped:
        raise RuntimeError("Bundled C++ runtime is not loaded")
    report = {"passed": True, "python": sys.version.split()[0], "wheel": str(Path(acc.__file__).resolve()),
              "jets": len(data["rows"]), "systematic_states": len(variations),
              "jvt_variations": 3, "release_setup": False}
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
