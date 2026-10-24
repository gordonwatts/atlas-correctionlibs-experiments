from pathlib import Path
import gzip
import json
import correctionlib
from atlas_correctionlib.systematics import Systematics
from atlas_correctionlib.jvt import Jvt

payload = Path('payloads')
engine = Systematics(payload)
assert engine._tables is None
tables = engine.tables
assert engine.tables is tables
assert set(tables.keys()) == set(correctionlib.CorrectionSet.from_file(str(payload/'uncertainty-tables.json.gz')).keys())
jvt = Jvt(payload, None)
assert jvt._ranges is None
with gzip.open(payload/'uncertainty-tables.json.gz', 'rt') as stream:
    data = json.load(stream)
expected = {}
for name in ['JvtFixedEffPt', 'EffFixedEffPt']:
    node = next(x for x in data['corrections'] if x['name'] == f'file2__{name}__None__content')['data']
    expected[name] = (node['edges'][0], node['edges'][-1], node['content'][0]['edges'][0], node['content'][0]['edges'][-1])
assert jvt.ranges == expected
assert jvt.ranges is jvt.ranges
print('PASS: lazy properties preserve payload tables and exact ranges; repeated access is cached')
