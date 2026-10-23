from pathlib import Path
root=Path('/home/gwatts/atlas/correctionlib_tests/c-nice')
p=root/'atlas_jet_corrections.py'
s=p.read_text().replace('import json, math','import json')
start=s.index('        for i, count in enumerate(expected):')
end=s.index('        values = ',start)
s=s[:start]+'''        counts = np.asarray(expected, dtype=np.int64)
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
''' +s[end:]
p.write_text(s)
win=Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib/c-nice')
(win/p.name).write_text(s)
