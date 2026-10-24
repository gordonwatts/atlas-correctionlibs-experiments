"""Four sequential full-sample runs per path; setup excluded, first run discarded."""
from pathlib import Path
import subprocess,time,json,statistics,shutil,sys,os
ROOT=Path("/home/gwatts/atlas/correctionlib_tests")
BASE=ROOT/"optimization-campaign/04-reuse-kinematics/runs"
def worker(path,iteration):
 work=BASE/(path+"-"+str(iteration));work.mkdir(parents=True,exist_ok=False)
 inputfile=Path("/cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt/src/AsgExternal/Asg_Test/cmt/TestFileLiteR3MC.txt").read_text().strip()
 if path in ("A","B"):
  commands=[["CPRun.py","-i",inputfile,"-t",str(ROOT/"config/reference.yaml"),"-o","output.root","-e","20000"]]
 else:
  (work/"scripts").mkdir();(work/"results").mkdir()
  for name in ("install","python","payloads"):(work/name).symlink_to(ROOT/name,target_is_directory=True)
  (work/"results/reference").symlink_to(ROOT/"results/reference",target_is_directory=True)
  for name in ("nominal","systematics","jvt"):shutil.copy2(ROOT/("c-nice" if name == "nominal" else "scripts")/("run-columnar-"+name+".py"),work/"scripts")
  commands=[
   [sys.executable,"scripts/run-columnar-nominal.py","--events","20000","--chunk-size","1000"],
   [sys.executable,"scripts/run-columnar-systematics.py","--events","20000","--output","results/columnar-kinematics.root"],
   [sys.executable,"scripts/run-columnar-jvt.py","--events","20000","--output","results/columnar-jvt.root","--kinematics","results/columnar-kinematics.root"]]
 stages=[];begin=time.perf_counter()
 for i,cmd in enumerate(commands):
  start=time.perf_counter()
  with (work/("stage-"+str(i)+".log")).open("w") as log:subprocess.run(cmd,cwd=work,stdout=log,stderr=subprocess.STDOUT,check=True)
  stages.append(time.perf_counter()-start)
 elapsed=time.perf_counter()-begin
 rec=dict(path=path,iteration=iteration,wall_seconds=elapsed,stage_seconds=stages,commands=commands,events=20000)
 (work/"timing.json").write_text(json.dumps(rec,indent=2))
 if path in ("A","B"):
  subprocess.run([sys.executable,str(ROOT/"scripts/compare-atlas.py"),"--candidate",str(work/"output.root"),"--report",str(work/"comparison.json")],check=True)
 else:
  for name in ("nominal","systematics","jvt"):assert json.loads((work/"results"/(name+"-comparison.json")).read_text())["passed"]
 print(json.dumps(rec),flush=True)
if len(sys.argv)>1:worker(sys.argv[1],int(sys.argv[2]));sys.exit()
BASE.mkdir(parents=True,exist_ok=True)
for iteration in range(1,5):
 for path in ("A","B","C"):
  record=BASE/(path+"-"+str(iteration))/"timing.json"
  if record.exists():raise RuntimeError("Existing timing run; use a new directory rather than mixing measurements")
  setup="setup-replacement.sh" if path=="B" else "setup.sh"
  cmd="cd "+str(ROOT)+"; source "+setup+" >/dev/null 2>&1; python scripts/repeated-timing.py "+path+" "+str(iteration)
  subprocess.run(["bash","-c",cmd],check=True)
records=[json.loads(p.read_text()) for p in sorted(BASE.glob("*/timing.json"))]
rows={}
for path in ("A","B","C"):
 values=[x["wall_seconds"] for x in sorted(records,key=lambda x:x["iteration"]) if x["path"]==path]
 rows[path]=dict(runs_seconds=values,average_seconds=statistics.mean(values[1:]),stdev_seconds=statistics.stdev(values[1:]))
report=dict(events=20000,jets=196606,run_order="A1 B1 C1 A2 B2 C2 A3 B3 C3 A4 B4 C4; sequential",discarded_run=1,rows=rows,records=records,scope="Wall time after environment setup; imports, initialization, input preparation, computation and output included. C also includes reference comparisons and recalculates kinematics in its JVT stage. A/B comparisons are outside timed jobs. These measure existing workflows, not isolated evaluator speed.",cache="No cache flushing; first run discarded per path; CVMFS/OS caches may already be warm.")
(BASE/"summary.json").write_text(json.dumps(report,indent=2))
lines=["# Repeated full-sample timings","","All runs: 20,000 events, 196,606 jets, atlas_al9 WSL2; AnalysisBase 25.2.100 toolchain. Seconds of wall-clock time.","","| Path | Run 1 (excluded) | Run 2 | Run 3 | Run 4 | Mean (2–4) | Sample SD (2–4) |","|---|---:|---:|---:|---:|---:|---:|"]
for path,label in [("A","A — untouched AnalysisBase"),("B","B — CorrectionLib ATLAS tools"),("C","C — columnar validation workflow")]:
 x=rows[path];lines.append("| "+label+" | "+" | ".join(f"{v:.2f}" for v in x["runs_seconds"]+[x["average_seconds"],x["stdev_seconds"]])+" |")
lines += ["",report["scope"],"",report["cache"],"","Each run has its own outputs and logs under optimization-campaign/04-reuse-kinematics/runs. All runs are checked against untouched AnalysisBase. See summary.json for commands, individual phase times, and unrounded measurements.","","Athena was not included: its previous test only initialized tools with zero events."]
(BASE/"summary.md").write_text("\n".join(lines)+"\n")
print("\n".join(lines))
