"""Repeat the original timing protocol with the array-preparation facade."""
from pathlib import Path
import hashlib, json, os, shutil, statistics, subprocess, sys, time

root = Path('/home/gwatts/atlas/correctionlib_tests')
base = root/'c-nice'
campaign = base/'array-timing'
campaign.mkdir(exist_ok=False)
shutil.copy2(base/'atlas_jet_corrections.py', campaign/'atlas_jet_corrections.py')
worker = (base/'timing.py').read_text().replace('c-nice/results', 'c-nice/array-timing/results')
(campaign/'worker.py').write_text(worker)
records = []
for i in range(1, 5):
    with (campaign/f'workflow-{i}.log').open('w') as log:
        subprocess.run([sys.executable, str(campaign/'worker.py'), 'C', str(i)], stdout=log, stderr=subprocess.STDOUT, check=True)
    work = campaign/f'results/C-{i}'
    for stage in ('nominal', 'systematics', 'jvt'):
        assert json.loads((work/f'results/{stage}-comparison.json').read_text())['passed']
    record = json.loads((work/'timing.json').read_text())
    records.append(record)
    print(f'Full workflow {i}: {record["wall_seconds"]:.2f} s; validation passed', flush=True)

plot_times = []
for i in range(1, 5):
    with (campaign/f'plot-{i}.log').open('w') as log:
        start = time.perf_counter()
        subprocess.run([sys.executable, str(base/'plot-jets.py')], cwd=campaign, env=dict(os.environ, MPLBACKEND='Agg'), stdout=log, stderr=subprocess.STDOUT, check=True)
        elapsed = time.perf_counter()-start
    plot_times.append(elapsed)
    print(f'Plot {i}: {elapsed:.2f} s', flush=True)

values = [r['wall_seconds'] for r in records]
summary = dict(workflow_runs_seconds=values, workflow_mean=statistics.mean(values[1:]), workflow_sd=statistics.stdev(values[1:]), plot_runs_seconds=plot_times, plot_mean=statistics.mean(plot_times[1:]), plot_sd=statistics.stdev(plot_times[1:]), records=records, events=20000, jets=196606, discarded_run=1, facade_sha256=hashlib.sha256((base/'atlas_jet_corrections.py').read_bytes()).hexdigest())
(campaign/'summary.json').write_text(json.dumps(summary, indent=2))
windows = Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib')
out = windows/'c-nice/array-timing'
out.mkdir(exist_ok=False)
for name in ('summary.json', 'worker.py', 'atlas_jet_corrections.py', 'jet-pt.png'):
    shutil.copy2(campaign/name, out/name)
for i in range(1, 5):
    checks = out/f'validation/run-{i}'
    checks.mkdir(parents=True)
    for stage in ('nominal', 'systematics', 'jvt'):
        shutil.copy2(campaign/f'results/C-{i}/results/{stage}-comparison.json', checks/f'{stage}.json')
doc = windows/'correctionlib-interface-handoff.md'
text = doc.read_text()
text = text.replace('The timing table below describes the earlier implementation with Python input-preparation loops; it has not been remeasured after this change.', 'The timing table below has been remeasured using this array-processing implementation.')
text = text.replace('| C-nice: facade nominal + C systematics/JVT | 188.53 | 2.31 | New measurement |', f'| C-nice: array facade nominal + C systematics/JVT | {summary["workflow_mean"]:.2f} | {summary["workflow_sd"]:.2f} | Remeasured after array update |')
text = text.replace('| C-nice: nominal file-to-Hist plot only | 28.45 | 0.13 | New measurement; smaller scope |', f'| C-nice: array nominal file-to-Hist plot only | {summary["plot_mean"]:.2f} | {summary["plot_sd"]:.2f} | Remeasured; smaller scope |')
text = text.replace('raw times and commands are saved in `c-nice/summary.json`', 'raw times and commands for the array implementation are saved in `c-nice/array-timing/summary.json` (the earlier measurements remain in `c-nice/summary.json`)')
text = text.replace('Plot installation and an initial plot/check run overlapped the discarded warm-up run.', 'The new measurements ran sequentially, with no plotting or dependency installation overlapping the full-workflow runs. The earlier pre-array means were 188.53 s for the full workflow and 28.45 s for the plot; these are separate campaigns, not an interleaved overhead study.')
doc.write_text(text)
(base/'interface-and-results.md').write_text(text)
readme = windows/'c-nice/README.md'
text = readme.read_text().replace('Saved timing results predate this update; the next campaign should measure both with and without systematics.', 'Remeasured timings for this update are in `array-timing/summary.json`; the original `summary.json` retains the earlier implementation timings. The full diagnostic workflow and the nominal-only plot have different output/validation scope; a future controlled campaign should compare with and without systematics under matched conditions.')
readme.write_text(text)
(base/'README.md').write_text(text)
shutil.copy2(Path(__file__), base/'remeasure-c-nice.py')
print(json.dumps(summary, indent=2), flush=True)
