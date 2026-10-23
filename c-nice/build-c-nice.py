from pathlib import Path
import textwrap

root = Path('/home/gwatts/atlas/correctionlib_tests')
dest = root / 'c-nice'
dest.mkdir(exist_ok=True)
original = (root / 'scripts/run-columnar-nominal.py').read_text()
preparation = original.split('    rows=[]\n', 1)[1].split('    allrows.extend(rows)', 1)[0]
preparation = preparation.replace('counts.append(n);events.append(int(arr[branches[0]][i]))', 'counts.append(n)')
preparation = preparation.replace('counts.append(n);events.append(int(arr["EventInfoAuxDyn.eventNumber"][i]))', 'counts.append(n)')
preparation = preparation.replace('len(arr[branches[0]])', 'len(event_columns["mu"])')
preparation = preparation.replace('        jets={f:', '        jets={f:')
header = '''"""C-nice: chunked nominal calibration using the unchanged B/C backend."""
from pathlib import Path
from types import SimpleNamespace
import json, math
import awkward as ak
import numpy as np
from _atlas_corrections import Nominal

FIELDS = ["JetConstitScaleMomentum_" + v for v in ("pt", "eta", "phi", "m")] + ["DetectorEta", "ActiveArea4vec_pt", "SumPtChargedPFOPt500", "EnergyPerSampling", "NumTrkPt1000", "TrackWidthPt1000"]
EVENT_BRANCHES = {"vertex_type": "PrimaryVerticesAuxDyn.vertexType", "rho": "Kt4EMPFlowNeutEventShapeAuxDyn.Density", "mu": "EventInfoAuxDyn.averageInteractionsPerCrossing", "event_number": "EventInfoAuxDyn.eventNumber"}

class JetCorrections:
    @classmethod
    def load(cls, bundle):
        obj = cls()
        obj.bundle = Path(bundle).resolve()
        settings = json.loads((obj.bundle / "nominal-manifest.json").read_text())["settings"]
        obj.engine = Nominal(str(obj.bundle / "nominal.json.gz"), settings["OffsetCorrection.DefaultMuRef"], settings["OffsetCorrection.DefaultNPVRef"], settings["MuScaleFactor"])
        return obj

    def evaluate(self, jet_columns, event_columns, *, systematics=("NOSYS",), outputs=("calibration",)):
        if tuple(systematics) != ("NOSYS",) or tuple(outputs) != ("calibration",):
            raise ValueError("C-nice currently supports only NOSYS calibration")
        for name in FIELDS:
            if name not in jet_columns:
                raise ValueError("Missing jet input: " + name)
        for name in EVENT_BRANCHES:
            if name not in event_columns:
                raise ValueError("Missing event input: " + name)
        expected = ak.to_numpy(ak.num(jet_columns[FIELDS[0]], axis=1))
        for name in FIELDS:
            if not np.array_equal(ak.to_numpy(ak.num(jet_columns[name], axis=1)), expected):
                raise ValueError("Inconsistent event/jet shape: " + name)
        for name in EVENT_BRANCHES:
            if len(event_columns[name]) != len(expected):
                raise ValueError("Inconsistent event count: " + name)
        for i, count in enumerate(expected):
            if not count:
                continue
            types = ak.to_list(event_columns["vertex_type"][i])
            if not types:
                raise ValueError("Jets require a nonempty vertex collection")
            pv = next((k for k,t in enumerate(types) if t == 1), 0)
            for name in ("SumPtChargedPFOPt500", "NumTrkPt1000", "TrackWidthPt1000"):
                if any(len(v) <= pv for v in ak.to_list(jet_columns[name][i])):
                    raise ValueError("Missing per-vertex input: " + name)
            if any(len(v) < 19 for v in ak.to_list(jet_columns["EnergyPerSampling"][i])):
                raise ValueError("EnergyPerSampling requires at least 19 samples")
        fields = FIELDS
        jprefix = "AnalysisJetsAuxDyn."
        arr = {jprefix + name: jet_columns[name] for name in FIELDS}
        arr.update({branch: event_columns[name] for name, branch in EVENT_BRANCHES.items()})
        rows, counts = [], []
'''
body = textwrap.indent(textwrap.dedent(preparation), '        ')
tail = '''        counts = np.asarray(counts, dtype=np.int64)
        values = np.asarray(self.engine.batch(rows), dtype=np.float64).reshape((-1, 12))
        valid = np.all(np.isfinite(values), axis=1)
        nominal = SimpleNamespace(**{name: ak.unflatten(values[:, 8+k], counts) for k, name in enumerate(("pt", "eta", "phi", "m"))}, valid=ak.unflatten(valid, counts))
        return {"NOSYS": nominal, "stages": values, "inputs": np.asarray(rows).reshape((-1, 14)), "counts": np.asarray(counts), "metadata": {"bundle": str(self.bundle), "systematics": ["NOSYS"], "validity": "finite outputs; backend exceptions propagate"}}
'''
(dest / 'atlas_jet_corrections.py').write_text(header + body + tail)
# Keep the existing diagnostic runner's output and reference comparison exactly.
start = original.index('manifest=json.loads')
end = original.index('testfile=')
runner = original[:start] + 'from atlas_jet_corrections import JetCorrections, EVENT_BRANCHES\nengine=JetCorrections.load(root/"payloads")\n' + original[end:]
start = runner.index('    rows=[]\n')
end = runner.index('    print("Processed', start)
runner = runner[:start] + '''    result = engine.evaluate({f: arr[jprefix+f] for f in fields}, {name: arr[branch] for name,branch in EVENT_BRANCHES.items()})
    counts.extend(result["counts"])
    events.extend(ak.to_list(arr["EventInfoAuxDyn.eventNumber"]))
    allrows.extend(result["inputs"])
    outputs.extend(result["stages"])
''' + runner[end:]
(dest / 'run-columnar-nominal.py').write_text(runner)
# Use the documented analysis code verbatim, substituting real paths.
doc = Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib/correctionlib-interface-handoff.md').read_text()
code = doc.split('### Worked analysis example:',1)[1].split('```python\n',1)[1].split('```',1)[0]
testfile = Path('/cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt/src/AsgExternal/Asg_Test/cmt/TestFileLiteR3MC.txt').read_text().strip()
code = code.replace('/path/to/pinned-run3-empflow-bundle',str(root/'payloads')).replace('/path/to/PHYSLITE.root',testfile).replace('plt.show()', 'plt.close(fig)')
(dest / 'plot-jets.py').write_text(code)
# Isolate every diagnostic output, preserving all previous acceptance results.
timing = (root/'scripts/repeated-timing.py').read_text()
timing = timing.replace('results/repeated-timing', 'c-nice/results')
timing = timing.replace('for name in ("nominal","systematics","jvt"):shutil.copy2(ROOT/"scripts"/("run-columnar-"+name+".py"),work/"scripts")', 'for name in ("nominal","systematics","jvt"):shutil.copy2(ROOT/("c-nice" if name == "nominal" else "scripts")/("run-columnar-"+name+".py"),work/"scripts")')
(dest/'timing.py').write_text(timing)
print('Created C-nice facade, diagnostic runner, plot example and isolated timing harness')
# Apply the array-preparation update whenever rebuilding the original scaffold.
import runpy
runpy.run_path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib/vectorize-c-nice.py')
