"""Run the complete diagnostic workflow in one process, retaining every check/output."""
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
