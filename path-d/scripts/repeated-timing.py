"""Four isolated D diagnostic runs; keep prior acceptance and timing artifacts untouched."""
from pathlib import Path
import json,time,subprocess,sys,shutil,statistics,hashlib
r=Path(__file__).resolve().parents[2];base=r/"path-d/results/repeated-timing";base.mkdir(exist_ok=True)
records=[]
for iteration in range(1,5):
 w=base/("run-"+str(iteration));w.mkdir(exist_ok=False)
 (w/"path-d/scripts").mkdir(parents=True);(w/"path-d/results").mkdir();(w/"results").mkdir()
 for name in ("build","python","payloads"):(w/"path-d"/name).symlink_to(r/"path-d"/name,target_is_directory=True)
 (w/"payloads").symlink_to(r/"payloads",target_is_directory=True)
 (w/"results/reference").symlink_to(r/"results/reference",target_is_directory=True)
 for name in ("nominal","systematics","jvt"):shutil.copy2(r/"path-d/scripts"/("run-"+name+".py"),w/"path-d/scripts")
 commands=[
 ["path-d/scripts/run-nominal.py","--events","20000","--chunk-size","1000"],
 ["path-d/scripts/run-systematics.py","--events","20000","--output","path-d/results/kinematics.root"],
 ["path-d/scripts/run-jvt.py","--events","20000","--output","path-d/results/jvt.root"]]
 phases={};start=time.perf_counter()
 for name,cmd in zip(("nominal","systematics","jvt"),commands):
  tick=time.perf_counter()
  with (w/(name+".log")).open("w") as log:subprocess.run([sys.executable,*cmd],cwd=w,stdout=log,stderr=subprocess.STDOUT,check=True)
  phases[name]=time.perf_counter()-tick
  print("Run",iteration,name,round(phases[name],2),"s",flush=True)
 elapsed=time.perf_counter()-start
 for name in phases:
  report=json.loads((w/"path-d/results"/(name+"-comparison.json")).read_text());assert report["passed"] and report["events"]==20000
 rec=dict(iteration=iteration,wall_seconds=elapsed,phases=phases,passed=True,commands=commands)
 (w/"timing.json").write_text(json.dumps(rec,indent=2));records.append(rec)
 print("Run",iteration,"total",round(elapsed,2),"s; validation PASS",flush=True)
kept=records[1:]
def stats(v):return dict(mean=statistics.mean(v),sample_sd=statistics.stdev(v))
summary=dict(path="D",events=20000,jets=196606,discarded_run=1,runs=records,total=stats([x["wall_seconds"] for x in kept]),phases={p:stats([x["phases"][p] for x in kept]) for p in ("nominal","systematics","jvt")},all_passed=True,scope="Sequential full diagnostic workflow after environment setup; includes imports, initialization, input/feature preparation, repeated kinematic evaluation, writing and reference comparisons. No cache flush; first run excluded. A/B/C timings are prior measurements, not interleaved with these D runs.")
(base/"summary.json").write_text(json.dumps(summary,indent=2))
old=json.loads((r/"results/repeated-timing/summary.json").read_text())
table=["| Metric | A: reference | B: ATLAS + CorrectionLib | C: columnar | D: declarative columnar |","|---|---|---|---|---|",
"| Implementation | Original ATLAS tools | Handwritten shared C++ physics + CorrectionLib payloads | Python input preparation + same shared C++ backend as B | Payload-defined physics + optional GraphSet extension and generic primitives |"]
for i in range(4):
 table.append("| Run "+str(i+1)+(" (excluded)" if i==0 else "")+" | "+" | ".join(f'{old["rows"][p]["runs_seconds"][i]:.2f} s' for p in ("A","B","C"))+f' | {records[i]["wall_seconds"]:.2f} s |')
table.append("| **Average, runs 2–4** | "+" | ".join(f'**{old["rows"][p]["average_seconds"]:.2f} s**' for p in ("A","B","C"))+f' | **{summary["total"]["mean"]:.2f} s** |')
table.append("| Sample SD, runs 2–4 | "+" | ".join(f'{old["rows"][p]["stdev_seconds"]:.2f} s' for p in ("A","B","C"))+f' | {summary["total"]["sample_sd"]:.2f} s |')
for idx,(p,label) in enumerate([("nominal","Nominal stage average"),("systematics","JES/JER stage average"),("jvt","NNJVT/weights stage average")]):
 c=statistics.mean(x["stage_seconds"][idx] for x in old["records"] if x["path"]=="C" and x["iteration"]>1)
 table.append(f'| {label} | Not separately measured | Not separately measured | {c:.2f} s | {summary["phases"][p]["mean"]:.2f} s |')
table+=["| Timed reference comparison | Excluded | Excluded | Included | Included |","| Retained-run validation | PASS | PASS | PASS | PASS |"]
text="# Four-path implementation and timing comparison\n\n20,000 events / 196,606 jets per run. All times are wall seconds. Each path was run four times; its first run is excluded from averages and sample standard deviations.\n\n"+"\n".join(table)+"\n\n"+summary["scope"]+"\n\nA/B process all stages within one ATLAS job, so separate phase costs are unavailable. C/D phase measurements include preparation, output and validation; the JVT phase repeats systematic evaluation. D timings use the diagnostic stage runners, not a new fused execution of the complete pipeline graphs. These are implemented-workflow costs, not isolated CorrectionLib evaluator benchmarks. Stage means can differ slightly from the total due to orchestration overhead.\n\nD is interpreted graph execution with a versioned optional extension, not stock schema-v2-only functionality or JIT compilation. ROOT coordinate/TRandom3 operations and lwtnn inference remain explicit reusable primitives.\n\nD raw measurements: path-d/results/repeated-timing/summary.json. A/B/C raw measurements: results/repeated-timing/summary.json. All isolated D run logs and outputs are retained beside its summary.\n"
(base/"summary.md").write_text(text)
(r/"results/four-path-timing-comparison.md").write_text(text)
p=r/"results/validation-summary.md";s=p.read_text().split("\n## Four-path timing comparison")[0];p.write_text(s+"\n## Four-path timing comparison\n\n"+text.split("\n",1)[1])
p=r/"path-d/results/validation-summary.md";s=p.read_text().split("\n## Repeated timing study")[0];p.write_text(s+"\n## Repeated timing study\n\n"+text.split("\n",1)[1])
print(text,flush=True)
