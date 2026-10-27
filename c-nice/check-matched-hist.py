"""Check the matched full-systematics pT histogram against saved Path A jets."""

import json
from pathlib import Path

import awkward as ak
import numpy as np
import uproot


ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "c-nice/matched-hist-timing/2026-10-27-01"
SAMPLE = Path(
    "/cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/"
    "InstallArea/x86_64-el9-gcc15-opt/src/AsgExternal/Asg_Test/cmt/"
    "TestFileLiteR3MC.txt"
).read_text().strip()


def counts_with_flow(pt_gev, constituent_eta, edges):
    selected = (pt_gev > 20.0) & (np.abs(constituent_eta) < 2.5)
    values = pt_gev[selected]
    bins = np.searchsorted(edges, values, side="right")
    return np.bincount(bins, minlength=len(edges) + 1)


def main():
    output = BASE / "full-4/jet-pt-full.npz"
    with np.load(output) as saved:
        names = saved["systematic_categories"].tolist()
        edges = saved["pt_edges_gev"]
        observed = saved["counts"]
        underflow = saved["underflow"]
        overflow = saved["overflow"]
        selected_counts = saved["jvt_selected_counts"]
    with uproot.open(SAMPLE) as source, \
         uproot.open(ROOT / "results/reference/reference.root") as reference:
        inputs = source["CollectionTree"].arrays([
            "AnalysisJetsAuxDyn.JetConstitScaleMomentum_eta",
            "EventInfoAuxDyn.eventNumber",
        ], how=dict)
        tree = reference["analysis"]
        if not np.array_equal(ak.to_numpy(inputs["EventInfoAuxDyn.eventNumber"]),
                              tree["eventNumber"].array(library="np")):
            raise AssertionError("Path A event identity differs from PHYSLITE")
        eta = inputs["AnalysisJetsAuxDyn.JetConstitScaleMomentum_eta"]
        shape = ak.to_numpy(ak.num(eta, axis=1))
        flat_eta = ak.to_numpy(ak.flatten(eta, axis=1))
        results = {}
        selection_failures = {}
        for index, name in enumerate(names):
            branch = "check_pt_" + name
            if name.startswith("JET_NNJvtEfficiency"):
                branch = "check_pt_NOSYS"
            reference_pt = tree[branch].array(library="ak")
            if not np.array_equal(ak.to_numpy(ak.num(reference_pt, axis=1)), shape):
                raise AssertionError(f"Path A jet counts differ for {name}")
            pt = ak.to_numpy(ak.flatten(reference_pt, axis=1)) / 1000.0
            expected = counts_with_flow(pt, flat_eta, edges)
            actual = np.concatenate(([underflow[index]], observed[:, index],
                                     [overflow[index]]))
            difference = actual - expected
            results[name] = dict(mismatched_bins=int(np.count_nonzero(difference)),
                                 max_abs_bin_difference=int(np.max(np.abs(difference))))
            selection_name = ("NOSYS" if name.startswith("JET_NNJvtEfficiency") else name)
            selected = tree["jet_select_jvt_" + selection_name].array(library="ak")
            if not np.array_equal(ak.to_numpy(ak.num(selected, axis=1)), shape):
                raise AssertionError(f"Path A JVT jet counts differ for {name}")
            expected_selected = int(ak.sum(selected))
            if int(selected_counts[index]) != expected_selected:
                selection_failures[name] = dict(actual=int(selected_counts[index]),
                                                reference=expected_selected)
    failures = {name: result for name, result in results.items()
                if result["mismatched_bins"]}
    report = dict(events=20000, jets=int(shape.sum()), categories=len(names),
                  mismatch_categories=len(failures), failures=failures,
                  jvt_selection_mismatch_categories=len(selection_failures),
                  jvt_selection_failures=selection_failures,
                  note="Histogram and JVT selection-count validation outside benchmark timer. "
                       "JVT weight sums are retained but not checked here; previous full-sample "
                       "C acceptance checks cover per-jet and event weights.",
                  passed=not failures and not selection_failures)
    (BASE / "reference-hist-check.json").write_text(json.dumps(report, indent=2))
    print(json.dumps(report, indent=2))
    if failures or selection_failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
