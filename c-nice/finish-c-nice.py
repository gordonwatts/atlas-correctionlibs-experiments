from pathlib import Path
import json, statistics, shutil, subprocess, sys, time, os
root=Path('/home/gwatts/atlas/correctionlib_tests')
base=root/'c-nice'
records=[json.loads((base/f'results/C-{i}/timing.json').read_text()) for i in range(1,5)]
for i in range(1,5):
    for stage in ('nominal','systematics','jvt'):
        assert json.loads((base/f'results/C-{i}/results/{stage}-comparison.json').read_text())['passed']
plot_times=[]
for i in range(4):
    begin=time.perf_counter()
    subprocess.run([sys.executable,str(base/'plot-jets.py')],cwd=base,env=dict(os.environ,MPLBACKEND='Agg'),check=True)
    plot_times.append(time.perf_counter()-begin)
values=[r['wall_seconds'] for r in records]
summary={'workflow_runs_seconds':values,'workflow_mean':statistics.mean(values[1:]),'workflow_sd':statistics.stdev(values[1:]),'plot_runs_seconds':plot_times,'plot_mean':statistics.mean(plot_times[1:]),'records':records}
(base/'summary.json').write_text(json.dumps(summary,indent=2))
windows=Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib')
out=windows/'c-nice';out.mkdir(exist_ok=True)
for name in ('atlas_jet_corrections.py','run-columnar-nominal.py','plot-jets.py','timing.py','summary.json','interface-checks.json','jet-pt.png'):
    shutil.copy2(base/name,out/name)
shutil.copy2(windows/'c-nice-README.md',out/'README.md')
shutil.copy2(windows/'c-nice-README.md',base/'README.md')
for i in range(1,5):
    checks=out/f'validation/run-{i}'
    checks.mkdir(parents=True,exist_ok=True)
    for stage in ('nominal','systematics','jvt'):
        shutil.copy2(base/f'results/C-{i}/results/{stage}-comparison.json',checks/f'{stage}.json')
for name in ('build-c-nice.py','check-c-nice.py','run-c-nice.sh','plot-c-nice.sh','get-plot-wheels.py','finish-c-nice.py'):
    shutil.copy2(windows/name,base/name)
doc=windows/'correctionlib-interface-handoff.md'
text=doc.read_text()
text=text.replace('**This is a design example, not a runnable example of the current implementation.**', '**The nominal-calibration slice below is now implemented as C-nice; see the implementation and timing section at the end.**')
text=text.replace('The `atlas_jet_corrections` import, input mapping and named result contract below are proposals to implement and validate.', 'The `atlas_jet_corrections` import, input mapping and named result contract below are implemented for nominal calibration on Path C. Other proposed interfaces remain future work.')
text=text.replace('# Proposed package.', '# C-nice package.').replace('# Proposed stage selection.', '# Nominal calibration only.').replace('# Proposed named result fields.', '# Named Awkward result fields.')
text=text.replace('Its nominal result has matching event-by-jet `pt` (MeV) and boolean `valid` arrays; `valid` is derived from the declared calibration status/validity contract.', 'Its nominal result has matching event-by-jet `pt` (MeV) and boolean `valid` arrays. In C-nice, `valid` checks finite stage outputs; backend exceptions propagate, and this does not establish a broader physics validity domain.')
text=text.replace('The facade owns primary-vertex selection, input preparation and unit/shape validation.', 'The facade owns primary-vertex selection, input preparation and collection-shape validation. Inputs must use the original PHYSLITE units; numerical arrays do not carry unit metadata.')
text += '\n## C-nice implementation and measured results (23 October 2026)\n\n'
text += 'The worked nominal-calibration example is now implemented in WSL at `/home/gwatts/atlas/correctionlib_tests/c-nice`. Windows inspection copies and the generated plot are in `c-nice/` beside this document. Load the existing `payloads/` directory as the bundle. The package requires `c-nice` and `install/python` on `PYTHONPATH` after sourcing the existing `setup.sh`. Run `c-nice/plot-jets.py` for the concrete file-to-histogram example. Plot dependencies installed: Hist 2.12.0 and mplhep 0.4.1.\n\n'
text += 'The facade loads the existing nominal manifest and shared `_atlas_corrections.Nominal` evaluator once, validates input names and collection lengths, prepares the same 14 inputs with the same float rounding as Path C, and returns named Awkward momentum and validity arrays preserving event boundaries. Empty chunks and zero-jet events are supported. Missing or malformed columns and unsupported requests raise errors. As in C, a nonempty vertex collection without a primary vertex uses index zero; an empty vertex collection with jets is explicitly rejected. `valid` means finite stage outputs with backend exceptions propagated; it is not an independently established physics-validity flag.\n\n'
text += 'No C++ physics, payload, binding or Path B adapter was modified. B continues to use exactly the shared backend used by this facade. Consequently B was not remeasured. The new facade currently supports only `systematics=["NOSYS"]` and `outputs=["calibration"]`; the broader recommended-systematics facade remains future work. The complete C-nice diagnostic workflow uses the new facade for nominal calibration followed by the existing C systematic and JVT runners.\n\n'
text += '| Path / workflow | Mean wall seconds | Sample SD | Measurement |\n|---|---:|---:|---|\n'
text += '| A: original ATLAS, full workflow | 237.26 | 0.96 | Previous baseline |\n| B: shared backend in ATLAS, full workflow | 290.70 | 1.01 | Previous baseline; backend unchanged |\n'
text += f'| C-nice: facade nominal + C systematics/JVT | {summary["workflow_mean"]:.2f} | {summary["workflow_sd"]:.2f} | New measurement |\n'
text += f'| C-nice: nominal file-to-Hist plot only | {summary["plot_mean"]:.2f} | {statistics.stdev(plot_times[1:]):.2f} | New measurement; smaller scope |\n\n'
text += 'All runs process 20,000 events / 196,606 jets. Each new workflow has four runs with the first excluded; raw times and commands are saved in `c-nice/summary.json`. Environment setup is outside timing. Full C-nice includes initialization, input preparation, saved intermediate/output files and reference comparisons, and retains C’s repeated kinematic evaluation in the JVT stage. A/B reference comparisons were outside their timers. The plotting timing includes startup, nominal evaluation, histogram filling and PNG output. A/B are historical runs, not interleaved measurements: these numbers do not isolate facade overhead or establish a controlled speedup. Plot installation and an initial plot/check run overlapped the discarded warm-up run.\n\n'
text += 'Validation: all four full workflows pass nominal, systematic and JVT comparisons against the original ATLAS reference. Additional interface checks establish bitwise equality between whole and chunked evaluation on 30 events, empty-chunk and zero-jet behavior, and rejection of missing columns, inconsistent shapes and unsupported systematic requests. Existing numerical tolerances are unchanged. The initial empty-chunk test exposed an untyped empty counts array; the facade now constructs integer counts explicitly. No new B numerical claim is made beyond its existing acceptance evidence.\n'
doc.write_text(text)
print(json.dumps(summary,indent=2))
