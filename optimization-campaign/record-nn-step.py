from pathlib import Path
import subprocess,json,shutil,sys,hashlib
r=Path('/home/gwatts/atlas/correctionlib_tests')
w=Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib')
label=sys.argv[1]
descriptions={
 '25-nn-feature-cache':('Cache exact NN input tuples in a bounded model-scoped native cache shared by B and C','src/nn_graph.hxx'),
 '26-nn-reusable-buffers':('Reuse NN intermediate buffers with matching Eigen operations in the shared B/C backend','src/nn_graph.hxx'),
 '12-rebuild-control':('Rebuild unchanged full-precision CorrectionLib with a reproducible Release configuration','vendor-patches/existing-full-precision-json.patch'),
 '13-bin-edge-reference':('Reference non-uniform bin edges instead of copying their vector on every lookup','vendor-patches/bin-edge-reference.patch'),
 '14-bound-tables':('Resolve shared systematic correction objects once at construction','src/systematics.hxx'),
 '15-paired-systematics':('Reuse exact shared uncertainty evaluation between related systematic states','src/systematics.hxx'),
 '18-exact-jvt-reuse':('Reuse JVT results only for exactly identical effective inputs','python/atlas_correctionlib/jvt.py'),
 '21-larger-read-chunks':('Read nominal inputs in 5000-event chunks instead of 1000; same full sample and outputs','scripts/run-columnar-full.py'),
 '20-flat-adapters':('Reuse one native input row and write scalar outputs directly into NumPy buffers','src/nominal.cxx'),
 '19-flat-group-buffers':('Write grouped native results directly into contiguous NumPy storage','src/nominal.cxx'),
 '17-batched-reference':('Read validation reference branches in bounded groups of 16','python/atlas_correctionlib/reference.py'),
 '16-integrated-workflow-v2':('Share loaded arrays and payloads between stages while retaining all outputs and checks','scripts/run-columnar-full.py'),
}
description,code=descriptions[label]
folder=r/'optimization-campaign'/label
summary=json.loads((folder/'summary.json').read_text());assert summary['passed']
for name in ('record-nn-step.py','build-nn-step.sh','check-nn-graph.cxx','check-nn-graph.py','check-nn-graph.sh','implement-nn-cache.py','implement-nn-buffers.py'):
 shutil.copy2(w/name,r/'optimization-campaign'/name)
lib=r/'venv/lib/python3.13/site-packages/correctionlib/lib/libcorrectionlib.so'
metadata=dict(library_sha256=hashlib.sha256(lib.read_bytes()).hexdigest(),library_source_commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=r/'external/correctionlib',text=True).strip(),single_core=True)
(folder/'implementation.json').write_text(json.dumps(metadata,indent=2))
subprocess.run(['git','add','.gitignore','src','python','scripts','atlas-overlay','c-nice','vendor-patches','optimization-campaign'],cwd=r,check=True)
subprocess.run(['git','commit','-q','-m',label+': '+description],cwd=r,check=True)
commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=r,text=True).strip()
dest=w/'optimization-results'/label;dest.mkdir(exist_ok=True)
for name in ('summary.json','implementation.json'):shutil.copy2(folder/name,dest/name)
for i in range(1,4):
 d=dest/f'run-{i}';d.mkdir(exist_ok=True)
 timing=folder/f'runs/C-{i}/results/full-stage-timing.json'
 if timing.exists():shutil.copy2(timing,d/'stage-timing.json')
 for stage in ('nominal','systematics','jvt'):
  shutil.copy2(folder/f'runs/C-{i}/results/{stage}-comparison.json',d/f'{stage}.json')
for base in ('src','python/atlas_correctionlib','scripts'):
 for p in (r/base).iterdir():
  if base=='scripts' and p.name not in ('run-columnar-nominal.py','run-columnar-systematics.py','run-columnar-jvt.py','run-columnar-full.py'):continue
  if p.is_file() and p.suffix in ('.py','.hxx','.cxx'):
   out=w/'columnar-source'/base/p.name;out.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,out)
shutil.copy2(r/'c-nice/timing.py',w/'c-nice/timing.py')
shutil.copytree(r/'vendor-patches',w/'vendor-patches',dirs_exist_ok=True)
index=w/'optimization-results/x2-index.json'
rows=json.loads(index.read_text()) if index.exists() else []
assert label not in [x['label'] for x in rows]
rows.append(dict(label=label,description=description,code=code,commit=commit,mean=summary['mean'],sd=summary['sd'],**metadata))
index.write_text(json.dumps(rows,indent=2))

shutil.copytree(r/'atlas-overlay/JetMomentTools',w/'columnar-source/atlas-overlay/JetMomentTools',dirs_exist_ok=True)
p=w/'x2-optimization.md';s=p.read_text();start=s.index('| Step |');end=s.index('\n\n',start)
row=rows[-1];link='columnar-source/'+row['code']
line=f"| {row['label']} | {row['mean']:.2f} ± {row['sd']:.2f} | {row['description']} [Source]({link}) | `{row['commit']}` |"
s=s[:end]+'\n'+line+s[end:];p.write_text(s)
print(commit,summary['mean'],flush=True)

