from pathlib import Path
r=Path('/home/gwatts/atlas/correctionlib_tests')
for name in ('systematics','jvt'):
 p=r/f'scripts/run-columnar-{name}.py';s=p.read_text()
 s=s.replace('stored=np.load(r/"results/columnar-nominal.npz")','stored=globals().get("shared_nominal")\nif stored is None:stored=np.load(r/"results/columnar-nominal.npz")')
 s=s.replace('t=uproot.open(inputfile)["CollectionTree"]','t=globals().get("shared_input_tree")\nif t is None:t=uproot.open(inputfile)["CollectionTree"]')
 s=s.replace('ref=uproot.open(r/a.reference)["analysis"]','ref=globals().get("shared_reference")\nif ref is None:ref=uproot.open(r/a.reference)["analysis"]')
 if name=='jvt':
  s=s.replace('engine=Systematics(r/"payloads");jvt=Jvt(r/"payloads",None);checks={}','engine=globals().get("shared_systematics")\nif engine is None:engine=Systematics(r/"payloads")\njvt=Jvt(r/"payloads",None);checks={}')
 p.write_text(s)
p=r/'scripts/run-columnar-full.py'
p.write_text('''"""Run the complete diagnostic workflow in one process, retaining every check/output."""
import argparse,json,sys,time
from pathlib import Path
import numpy as np

root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser()
parser.add_argument("--events",type=int,default=100)
parser.add_argument("--chunk-size",type=int,default=1000)
args=parser.parse_args()
times={}

def stage(name,arguments,shared=None):
    path=root/"scripts"/("run-columnar-"+name+".py")
    namespace={"__name__":"__main__","__file__":str(path),**(shared or {})}
    previous=sys.argv
    sys.argv=[str(path),*arguments]
    begin=time.perf_counter()
    try:
        try:exec(compile(path.read_text(),str(path),"exec"),namespace)
        except SystemExit as error:
            if error.code not in (None,0):raise
    finally:
        sys.argv=previous
        times[name]=time.perf_counter()-begin
    return namespace

nominal=stage("nominal",["--events",str(args.events),"--chunk-size",str(args.chunk_size)])
shared={"shared_nominal":{"values":nominal["out"],"counts":np.asarray(nominal["counts"]),"eventNumber":np.asarray(nominal["events"])},
        "shared_input_tree":nominal["tree"],"shared_reference":nominal["reference"]}
del nominal
systematics=stage("systematics",["--events",str(args.events),"--output","results/columnar-kinematics.root"],shared)
shared["shared_systematics"]=systematics["engine"]
del systematics
# Keep the saved-kinematics reader and its provenance/identity/status checks.
stage("jvt",["--events",str(args.events),"--output","results/columnar-jvt.root","--kinematics","results/columnar-kinematics.root"],shared)
(root/"results/full-stage-timing.json").write_text(json.dumps(times,indent=2))
''')
p=r/'c-nice/timing.py';s=p.read_text()
s=s.replace('for name in ("nominal","systematics","jvt"):shutil.copy2','for name in ("nominal","systematics","jvt","full"):shutil.copy2')
start=s.index('  commands=[\n   [sys.executable,');end=s.index('\n stages=[];',start)
s=s[:start]+'''  commands=[[sys.executable,"scripts/run-columnar-full.py","--events","20000","--chunk-size","1000"]]'''+s[end:]
p.write_text(s)
