#!/usr/bin/env python3
"""Freeze a small acceptance fixture from the existing release-backed evaluator."""
import argparse
from pathlib import Path
import sys

import numpy as np


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--jets", type=int, default=12)
    args = parser.parse_args()
    root = args.root.resolve()
    output = args.output.resolve()
    if output.exists():
        parser.error(f"fixture already exists: {output}")
    sys.path[:0] = [str(root / "install/python"), str(root / "python")]
    from _atlas_corrections import Nominal
    from atlas_correctionlib.systematics import Systematics
    from atlas_correctionlib.jvt import Jvt
    import json

    rows = np.load(root / "results/columnar-inputs.npz")["rows"][:args.jets]
    settings = json.loads((root / "payloads/nominal-manifest.json").read_text())["settings"]
    nominal = Nominal(str(root / "payloads/nominal.json.gz"),
                      settings["OffsetCorrection.DefaultMuRef"],
                      settings["OffsetCorrection.DefaultNPVRef"],
                      settings["MuScaleFactor"])
    nominal_expected = np.asarray(nominal.batch_array(rows))
    inputs = np.load(root / "results/jvt-inputs.npz")
    p4, mu, npv, bjet = (inputs[name][:args.jets] for name in ("p4", "mu", "npv", "bjet"))
    sys_engine = Systematics(root / "payloads")
    variations = list(sys_engine.variations(p4, mu, npv, bjet))
    names = np.asarray([name for name, _ in variations])
    values = np.stack([value for _, value in variations])
    jvt = Jvt(root / "payloads", None)
    jvt_inputs = {name: inputs[name][:args.jets] for name in ("sumpt", "jvfcorr", "haspv", "ishs")}
    score, selected, weights, valid = jvt.evaluate(p4, **jvt_inputs)
    up = jvt.evaluate(p4, **jvt_inputs, sigma=1)
    down = jvt.evaluate(p4, **jvt_inputs, sigma=-1)
    output.parent.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(output, rows=rows, nominal=nominal_expected,
                        p4=p4, mu=mu, npv=npv, bjet=bjet,
                        names=names, variations=values,
                        jvt_score=score, jvt_selected=selected,
                        jvt_weights=weights, jvt_valid=valid,
                        jvt_up_weights=up[2], jvt_down_weights=down[2],
                        **jvt_inputs)
    print(f"{len(rows)} nominal jets, {len(names)} systematic states, {len(score)} JVT jets -> {output}")


if __name__ == "__main__":
    main()
