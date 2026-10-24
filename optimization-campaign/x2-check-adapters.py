from pathlib import Path
import numpy as np
exec(Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib/check-array-bindings.py').read_text())
from atlas_jet_corrections import JetCorrections
engine=JetCorrections.load(root/'payloads').engine
rows=np.load(root/'optimization-campaign/17-batched-reference/runs/C-2/results/columnar-inputs.npz')['rows'][:1024]
expected=np.asarray(engine.batch(rows.tolist()))
actual=engine.batch_array(rows)
assert actual.dtype==expected.dtype and actual.tobytes()==expected.tobytes()
assert np.array_equal(engine.batch_array(rows[::2]),expected[::2])
assert engine.batch_array(np.empty((0,14))).shape==(0,12)
print('Nominal legacy/array raw-byte parity, strided/empty batches PASS')
