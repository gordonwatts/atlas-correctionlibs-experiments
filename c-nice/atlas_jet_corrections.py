"""C-nice: chunked nominal calibration using the unchanged B/C backend."""
from pathlib import Path
from types import SimpleNamespace
import json
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
        counts = np.asarray(expected, dtype=np.int64)
        types = event_columns["vertex_type"]
        if bool(ak.any((ak.num(types, axis=1) == 0) & (counts > 0))):
            raise ValueError("Jets require a nonempty vertex collection")
        # First primary vertex, otherwise index zero: identical to Path C.
        pv = ak.fill_none(ak.firsts(ak.local_index(types)[types == 1]), 0)
        flat_pv = np.repeat(ak.to_numpy(pv).astype(np.int64), counts)
        npv = ak.to_numpy(ak.sum((types == 1) | (types == 3), axis=1))

        def flat(name):
            return ak.to_numpy(ak.flatten(jet_columns[name], axis=1)).astype(np.float64)

        def at_vertex(name):
            column = ak.flatten(jet_columns[name], axis=1)
            if bool(ak.any(ak.num(column, axis=1) <= flat_pv)):
                raise ValueError("Missing per-vertex input: " + name)
            selected = column[ak.local_index(column, axis=1) == flat_pv[:, None]]
            return ak.to_numpy(ak.firsts(selected)).astype(np.float64)

        pt, eta, phi, mass = (flat("JetConstitScaleMomentum_" + x) for x in ("pt", "eta", "phi", "m"))
        samples = ak.flatten(jet_columns["EnergyPerSampling"], axis=1)
        if bool(ak.any(ak.num(samples, axis=1) < 19)):
            raise ValueError("EnergyPerSampling requires at least 19 samples")
        # Preserve the original float32 rounding at energy and fraction steps.
        energy = np.sqrt((pt * np.cosh(eta)) ** 2 + mass * mass).astype(np.float32)
        def sample(index):
            return ak.to_numpy(samples[:, index]).astype(np.float32)
        with np.errstate(divide="raise", invalid="raise", over="raise"):
            charged = at_vertex("SumPtChargedPFOPt500") / pt
            tile = (sample(12) + sample(18)) / energy
            em3 = (sample(3) + sample(7)) / energy
        rows = np.column_stack((
            pt, eta, phi, mass, flat("DetectorEta"), flat("ActiveArea4vec_pt"),
            np.repeat(ak.to_numpy(event_columns["rho"]).astype(np.float64), counts),
            np.repeat(ak.to_numpy(event_columns["mu"]).astype(np.float64), counts),
            np.repeat(npv, counts), charged, tile, em3,
            at_vertex("NumTrkPt1000"), at_vertex("TrackWidthPt1000"),
        ))
        values = np.asarray(self.engine.batch_array(rows), dtype=np.float64).reshape((-1, 12))
        valid = np.all(np.isfinite(values), axis=1)
        nominal = SimpleNamespace(**{name: ak.unflatten(values[:, 8+k], counts) for k, name in enumerate(("pt", "eta", "phi", "m"))}, valid=ak.unflatten(valid, counts))
        return {"NOSYS": nominal, "stages": values, "inputs": np.asarray(rows).reshape((-1, 14)), "counts": np.asarray(counts), "metadata": {"bundle": str(self.bundle), "systematics": ["NOSYS"], "validity": "finite outputs; backend exceptions propagate"}}
