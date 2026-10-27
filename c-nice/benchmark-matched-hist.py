"""Time matched C-nice nominal and full-systematics file-to-histogram runs."""

import json
import os
from pathlib import Path
import statistics
import subprocess
import sys
import time

import numpy as np


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "c-nice/plot-jets-matched.py"
BASE = ROOT / "c-nice/matched-hist-timing/2026-10-27-01"
SAMPLE_LIST = Path(
    "/cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/"
    "InstallArea/x86_64-el9-gcc15-opt/src/AsgExternal/Asg_Test/cmt/"
    "TestFileLiteR3MC.txt"
)


def main():
    if BASE.exists():
        raise RuntimeError(f"Refusing to overwrite existing campaign: {BASE}")
    sample = Path(SAMPLE_LIST.read_text().strip())
    if not sample.is_file():
        raise FileNotFoundError(sample)
    BASE.mkdir(parents=True)
    allowed = os.sched_getaffinity(0)
    cpu = min(allowed)
    os.sched_setaffinity(0, {cpu})
    env = dict(os.environ, ATLAS_CORRECTIONLIB_THREADS="1", OPENBLAS_NUM_THREADS="1",
               OMP_NUM_THREADS="1", MKL_NUM_THREADS="1")
    env["PYTHONPATH"] = os.pathsep.join((str(ROOT / "c-nice"), str(ROOT / "python"),
                                        str(ROOT / "install/python"), env.get("PYTHONPATH", "")))
    records = []
    for iteration in range(1, 5):
        for mode in ("nominal", "full"):
            work = BASE / f"{mode}-{iteration}"
            work.mkdir()
            command = [sys.executable, str(SCRIPT), str(sample), "--mode", mode]
            start = time.perf_counter()
            with (work / "run.log").open("w") as log:
                subprocess.run(command, cwd=work, env=env, stdout=log,
                               stderr=subprocess.STDOUT, check=True)
            wall = time.perf_counter() - start
            result = work / f"jet-pt-{mode}.npz"
            with np.load(result) as data:
                record = dict(mode=mode, iteration=iteration, wall_seconds=wall,
                              events=int(data["events"]), jets=int(data["jets"]),
                              categories=len(data["systematic_categories"]),
                              total_entries=int(data["total_entries"]),
                              command=command)
            (work / "timing.json").write_text(json.dumps(record, indent=2))
            records.append(record)
            print(json.dumps(record), flush=True)
    summaries = {}
    for mode in ("nominal", "full"):
        rows = [r for r in records if r["mode"] == mode]
        values = [r["wall_seconds"] for r in rows[1:]]
        summaries[mode] = dict(mean=statistics.mean(values), sd=statistics.stdev(values),
                               runs=[r["wall_seconds"] for r in rows])
    for iteration in range(1, 5):
        with np.load(BASE / f"nominal-{iteration}/jet-pt-nominal.npz") as nominal, \
             np.load(BASE / f"full-{iteration}/jet-pt-full.npz") as full:
            if list(full["systematic_categories"])[0] != "NOSYS":
                raise AssertionError("Full run first category is not NOSYS")
            for key in ("counts", "underflow", "overflow"):
                if not np.array_equal(nominal[key][:, 0] if key == "counts" else nominal[key][0],
                                      full[key][:, 0] if key == "counts" else full[key][0]):
                    raise AssertionError(f"Nominal histogram mismatch: {key}, run {iteration}")
            if int(nominal["events"]) != 20000 or int(full["events"]) != 20000:
                raise AssertionError("Unexpected event count")
            if int(nominal["jets"]) != 196606 or int(full["jets"]) != 196606:
                raise AssertionError("Unexpected jet count")
            if len(full["systematic_categories"]) != 119:
                raise AssertionError("Unexpected systematic category count")
    summary = dict(rows=summaries, records=records, cpu=cpu, discarded_run=1,
                   nominal_histograms_equal_across_modes=True,
                   scope="Same input, branch list, chunking, selection and pT binning; full mode "
                         "adds 117 kinematic states and two NNJVT-efficiency states. "
                         "Process startup, reading, correction, histogram and NPZ output included. "
                         "No saved-reference comparison in timed runs; one CPU; no cache flush.")
    (BASE / "summary.json").write_text(json.dumps(summary, indent=2))
    print(json.dumps(summary["rows"], indent=2), flush=True)


if __name__ == "__main__":
    main()
