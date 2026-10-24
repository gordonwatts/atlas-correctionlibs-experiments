"""Validated reuse of systematic-stage outputs; no correction arithmetic here."""
import hashlib
import json
from pathlib import Path
import awkward as ak
import numpy as np
import uproot
import _atlas_corrections

def fingerprint(payload, **arrays):
    digest=hashlib.sha256()
    for name in sorted(arrays):
        a=np.ascontiguousarray(arrays[name])
        digest.update(name.encode())
        digest.update(str((a.shape,a.dtype.str)).encode())
        digest.update(a.tobytes())
    for path in (Path(payload)/"uncertainty-manifest.json",Path(payload)/"uncertainty-tables.json.gz",Path(_atlas_corrections.__file__)):
        with path.open("rb") as stream:
            for block in iter(lambda:stream.read(1024*1024),b""):
                digest.update(block)
    return digest.hexdigest()

def read(path, expected, names, counts, events):
    with uproot.open(path) as source:
        metadata=json.loads(str(source["__kinematic_cache__"]))
        if metadata != {"version":1,"fingerprint":expected,"names":names}:
            raise ValueError("Kinematic cache does not match inputs, payload, backend or systematic set")
        for name in names:
            a=source[name].arrays(library="ak",how=dict)
            if not np.array_equal(ak.to_numpy(a["eventNumber"]),events):
                raise ValueError("Cached event identities differ")
            if not np.array_equal(ak.to_numpy(ak.num(a["pt"])),counts):
                raise ValueError("Cached jet counts differ")
            if not bool(ak.all(a["originalJetIndex"]==ak.local_index(a["pt"],axis=1))):
                raise ValueError("Cached jet ordering differs")
            if not bool(ak.all(a["status"]==2)):
                raise ValueError("Cached corrections contain unsuccessful status")
            yield name,np.column_stack([ak.to_numpy(ak.flatten(a[x])) for x in ("pt","eta","phi","m")])
