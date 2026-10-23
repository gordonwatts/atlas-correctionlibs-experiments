from pathlib import Path
import hashlib,subprocess,sys,json
r=Path(__file__).resolve().parents[2]
p=r/"path-d/payloads/graphs.json"
before=hashlib.sha256(p.read_bytes()).hexdigest()
subprocess.run([sys.executable,str(r/"path-d/python/compile_graphs.py")],check=True)
after=hashlib.sha256(p.read_bytes()).hexdigest()
assert before==after,(before,after)
report=dict(passed=True,graph_sha256=after,byte_identical_regeneration=True)
(r/"path-d/results/reproducibility.json").write_text(json.dumps(report,indent=2))
print(report)
