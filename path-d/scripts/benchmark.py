"""One post-acceptance workflow timing; not an isolated numerical-kernel benchmark."""
import subprocess,time,resource,json,sys
from pathlib import Path
r=Path(__file__).resolve().parents[2];d=r/"path-d";out=d/"results"
for name in ["nominal-comparison","systematics-comparison","jvt-comparison","systematic-determinism","uncertainty-boundaries","primitives","pipelines","table-edges"]:
 assert json.loads((out/(name+".json")).read_text())["passed"],name
commands=[
 ["path-d/scripts/run-nominal.py","--events","20000","--chunk-size","1000"],
 ["path-d/scripts/run-systematics.py","--events","20000","--output","path-d/results/kinematics.root"],
 ["path-d/scripts/run-jvt.py","--events","20000","--output","path-d/results/jvt.root"]]
phases=[];total=time.perf_counter()
for label,command in zip(["nominal","systematics","jvt"],commands):
 start=time.perf_counter()
 with (out/("timing-"+label+".log")).open("w") as log:
  subprocess.run([sys.executable,*command],cwd=r,stdout=log,stderr=subprocess.STDOUT,check=True)
 phases.append(dict(name=label,seconds=time.perf_counter()-start,command=[sys.executable,*command]))
report=dict(path="D",events=20000,jets=196606,runs=1,wall_seconds=time.perf_counter()-total,phases=phases,peak_child_rss_kib=resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss,scope="Complete three-stage diagnostic workflow after environment setup, including imports, input/feature preparation, repeated kinematic evaluation, output and reference comparisons. Warm caches possible. Single run, not an isolated kernel or repeated timing estimate.")
(out/"timing.json").write_text(json.dumps(report,indent=2))
(out/"timing.md").write_text("# Path D workflow timing\n\nOne post-correctness run: **%.2f s**, 20,000 events / 196,606 jets.\n\n%s\n\n| Phase | Wall seconds |\n|---|---:|\n%s\n\nThe earlier A/B/C means (237.26 / 290.70 / 167.70 s) use four runs with the first discarded. D is a single differently structured diagnostic workflow, so this is not a controlled evaluator-speed comparison.\n"%(report["wall_seconds"],report["scope"],"\n".join("| %s | %.2f |"%(x["name"],x["seconds"]) for x in phases)))
print(json.dumps(report,indent=2))
