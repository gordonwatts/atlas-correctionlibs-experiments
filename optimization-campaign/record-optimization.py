from pathlib import Path
import json, shutil, subprocess, sys
root=Path('/home/gwatts/atlas/correctionlib_tests')
windows=Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib')
label=sys.argv[1]
descriptions={
'00-baseline':('Fresh array-preparation baseline; unchanged evaluator and diagnostic outputs','c-nice/atlas_jet_corrections.py'),
'01-array-bindings':('Native dense-array batch bindings eliminate Python scalar/list transfer; shared evaluators unchanged','src/nominal.cxx'),
'02-event-products':('Replace per-event Python weight products with float64 Awkward reductions','scripts/run-columnar-jvt.py'),
'03-compiled-metadata':('Parse uncertainty component metadata and reference constants once at construction','src/systematics.hxx'),
'04-reuse-kinematics':('Reuse validated kinematics from the systematic stage in the JVT stage','scripts/run-columnar-jvt.py'),
'05-parallel-batches':('Evaluate independent native batch slices on four threads; preserve scalar physics and ordering','python/atlas_correctionlib/parallel.py'),
'06-array-jvt-inputs':('Replace Python event loops for primary-vertex inputs and truth matching with Awkward arrays','scripts/run-columnar-jvt.py'),
'07-lz4-output':('Use LZ4 for the same ROOT output columns instead of default ZLIB','scripts/run-columnar-systematics.py'),
}
description,ref=descriptions[label]
folder=root/'optimization-campaign'/label
summary=json.loads((folder/'summary.json').read_text())
assert summary['passed']
for name in ('optimization-campaign.py','optimization-campaign.sh','record-optimization.py'):
    shutil.copy2(windows/name,root/'optimization-campaign'/name)
subprocess.run(['git','add','src','python','scripts','c-nice/atlas_jet_corrections.py','c-nice/timing.py','optimization-campaign'],cwd=root,check=True)
subprocess.run(['git','commit','-q','-m',f'{label}: {description}'],cwd=root,check=True)
commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
out=windows/'optimization-results'/label
out.mkdir(parents=True,exist_ok=True)
shutil.copy2(folder/'summary.json',out/'summary.json')
for i in range(1,4):
    dest=out/f'run-{i}';dest.mkdir(exist_ok=True)
    for stage in ('nominal','systematics','jvt'):
        shutil.copy2(folder/f'runs/C-{i}/results/{stage}-comparison.json',dest/f'{stage}.json')
for rel in ('src/nominal.cxx','src/systematics.hxx','scripts/run-columnar-jvt.py','scripts/run-columnar-systematics.py','python/atlas_correctionlib/systematics.py','python/atlas_correctionlib/jvt.py','python/atlas_correctionlib/parallel.py','python/atlas_correctionlib/kinematic_cache.py'):
    if not (root/rel).exists(): continue
    dest=windows/'columnar-source'/rel;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(root/rel,dest)
shutil.copy2(root/'c-nice/atlas_jet_corrections.py',windows/'c-nice/atlas_jet_corrections.py')
shutil.copy2(root/'c-nice/timing.py',windows/'c-nice/timing.py')
index=windows/'optimization-results/index.json'
records=json.loads(index.read_text()) if index.exists() else []
records.append(dict(label=label,description=description,code=ref,commit=commit,mean=summary['mean'],sd=summary['sd']))
index.write_text(json.dumps(records,indent=2))
lines=['# C-nice full-systematics optimization campaign','','20,000 events / 196,606 jets; all 117 kinematic and 119 JVT/weight states. Three sequential runs per step; first discarded, mean and sample SD of the remaining two. Setup and compilation excluded. Existing imports, input preparation, numerical evaluation, ROOT outputs and A-reference comparisons remain included. No random-seed changes. Timing differences include run-to-run machine variation.','','| Step | Full C-nice wall seconds | Change and code reference | WSL implementation commit |','|---|---:|---|---|']
for r in records:
    link=r['code'] if r['code'].startswith('c-nice/') else 'columnar-source/'+r['code']
    lines.append(f'| {r["label"]} | {r["mean"]:.2f} ± {r["sd"]:.2f} | {r["description"]}; [source]({link}) | `{r["commit"]}` |')
lines += ['','Every row passed all full-sample nominal, systematic and JVT reference comparisons. Per-run reports and raw timings are retained in `optimization-results/`. The facade remains nominal-only; full systematic timings refer to its nominal stage followed by the established C systematic/JVT runners. B shares the physics implementation. B configuration is unchanged; any shared-backend change also requires its own B integration validation before acceptance.']
integration=root/'optimization-campaign/03-B-integration'
if (integration/'summary.json').exists():
    b=json.loads((integration/'summary.json').read_text())
    destination=windows/'optimization-results/B-integration';destination.mkdir(exist_ok=True)
    for name in ('summary.json','comparison.json','loaded-libraries.txt'):
        shutil.copy2(integration/name,destination/name)
    lines += ['',f'B integration after step 03: all 20,000 events passed comparison with A using unchanged `config/reference.yaml` (SHA-256 `{b["config_sha256"]}`). One run took {b["wall_seconds"]:.2f} s; this is an integration check, not a repeated speedup measurement. Saved worker memory mappings confirm the rebuilt overlay libraries were loaded. Subsequent Python-only steps leave this shared C++ implementation unchanged.']
(windows/'c-nice-optimization.md').write_text('\n'.join(lines)+'\n')
print(commit,summary['mean'],flush=True)
