"""C-nice: chunked nominal calibration using the unchanged B/C backend."""
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
        for i in range(len(event_columns["mu"])):
            types=ak.to_list(arr["PrimaryVerticesAuxDyn.vertexType"][i])
            pv=next((k for k,t in enumerate(types) if t==1),0)
            npv=sum(t in (1,3) for t in types)
            rho=float(arr["Kt4EMPFlowNeutEventShapeAuxDyn.Density"][i])
            mu=float(arr["EventInfoAuxDyn.averageInteractionsPerCrossing"][i])
            jets={f:ak.to_list(arr[jprefix+f][i]) for f in fields}
            n=len(jets["DetectorEta"]);counts.append(n)
            for j in range(n):
                pt,eta,phi,m=(float(jets["JetConstitScaleMomentum_"+x][j]) for x in ["pt","eta","phi","m"])
                energy=np.float32(math.sqrt((pt*math.cosh(eta))**2+m*m))
                sample=np.asarray(jets["EnergyPerSampling"][j],dtype=np.float32)
                tile=float(np.float32((sample[12]+sample[18])/energy))
                em3=float(np.float32((sample[3]+sample[7])/energy))
                charged=float(jets["SumPtChargedPFOPt500"][j][pv])/pt
                rows.append([pt,eta,phi,m,float(jets["DetectorEta"][j]),float(jets["ActiveArea4vec_pt"][j]),rho,mu,float(npv),charged,tile,em3,float(jets["NumTrkPt1000"][j][pv]),float(jets["TrackWidthPt1000"][j][pv])])
        counts = np.asarray(counts, dtype=np.int64)
        values = np.asarray(self.engine.batch(rows), dtype=np.float64).reshape((-1, 12))
        valid = np.all(np.isfinite(values), axis=1)
        nominal = SimpleNamespace(**{name: ak.unflatten(values[:, 8+k], counts) for k, name in enumerate(("pt", "eta", "phi", "m"))}, valid=ak.unflatten(valid, counts))
        return {"NOSYS": nominal, "stages": values, "inputs": np.asarray(rows).reshape((-1, 14)), "counts": np.asarray(counts), "metadata": {"bundle": str(self.bundle), "systematics": ["NOSYS"], "validity": "finite outputs; backend exceptions propagate"}}
