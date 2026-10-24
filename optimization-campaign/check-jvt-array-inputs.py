from pathlib import Path
import json
import numpy as np
root=Path('/home/gwatts/atlas/correctionlib_tests/optimization-campaign')
before=np.load(root/'05-parallel-batches/runs/C-3/results/jvt-inputs.npz')
after=np.load(root/'06-array-jvt-inputs/runs/C-3/results/jvt-inputs.npz')
assert before.files==after.files
checks={key:bool(np.array_equal(before[key],after[key])) for key in before.files}
assert all(checks.values()),checks
(root/'checks/jvt-array-inputs.json').write_text(json.dumps(checks,indent=2))
print('All JVT prepared arrays bitwise identical on the full sample:',checks)
