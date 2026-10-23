"""Use system Python for HTTPS; ATLAS Python in this release needs newer OpenSSL."""
from pathlib import Path
import urllib.request,json,subprocess,os
r=Path(__file__).resolve().parents[1]
(r/"external/wheels").mkdir(parents=True,exist_ok=True)
meta=json.load(urllib.request.urlopen("https://pypi.org/pypi/pip/25.3/json"))
item=next(x for x in meta["urls"] if x["filename"].endswith(".whl"))
wheel=r/"external"/item["filename"]
urllib.request.urlretrieve(item["url"],wheel)
env=dict(os.environ,PYTHONPATH=str(wheel))
subprocess.run(["python3","-m","pip","download","--dest",str(r/"external/wheels"),"--python-version","313","--abi","cp313","--only-binary=:all:","-r",str(r/"config/requirements-wheels.lock")],env=env,check=True)
