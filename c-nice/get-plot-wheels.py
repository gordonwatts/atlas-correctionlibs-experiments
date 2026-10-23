import json, urllib.request
from pathlib import Path
out=Path('/home/gwatts/atlas/correctionlib_tests/c-nice/wheels')
out.mkdir(exist_ok=True)
for name in ['hist','boost-histogram','histoprint','click','uhi','mplhep','mplhep-data']:
    data=json.load(urllib.request.urlopen('https://pypi.org/pypi/'+name+('/0.4.1' if name == 'mplhep' else '')+'/json'))
    candidates=[x for x in data['urls'] if x['filename'].endswith('py3-none-any.whl') or ('cp313-cp313-manylinux' in x['filename'] and 'x86_64' in x['filename'])]
    x=candidates[0]
    urllib.request.urlretrieve(x['url'],out/x['filename'])
    print(x['filename'])
