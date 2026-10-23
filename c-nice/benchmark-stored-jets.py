from pathlib import Path
import json, os, shutil, statistics, subprocess, sys, time

base = Path('/home/gwatts/atlas/correctionlib_tests/c-nice')
windows = Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib')
shutil.copy2(windows/'c-nice/plot-stored-jets.py', base/'plot-stored-jets.py')
campaign = base/'stored-timing'
campaign.mkdir(exist_ok=False)
records = []
reference = None
for iteration in range(1, 5):
    for mode in ('minimal', 'extra-inputs'):
        work = campaign/f'{mode}-{iteration}'
        work.mkdir()
        command = [sys.executable, str(base/'plot-stored-jets.py')]
        if mode == 'extra-inputs':
            command += ['--read-calibration-inputs']
        with (work/'run.log').open('w') as log:
            start = time.perf_counter()
            subprocess.run(command, cwd=work, env=dict(os.environ, MPLBACKEND='Agg'), stdout=log, stderr=subprocess.STDOUT, check=True)
            elapsed = time.perf_counter()-start
        result = json.loads((work/'histogram.json').read_text())
        if reference is None:
            reference = result['counts_including_flow']
        assert reference == result['counts_including_flow']
        record = dict(mode=mode, iteration=iteration, wall_seconds=elapsed, command=command)
        records.append(record)
        (work/'timing.json').write_text(json.dumps(record, indent=2))
        print(f'{mode} {iteration}: {elapsed:.3f} s; counts verified', flush=True)
rows = {}
for mode in ('minimal', 'extra-inputs'):
    values = [r['wall_seconds'] for r in records if r['mode'] == mode]
    rows[mode] = dict(runs_seconds=values, mean=statistics.mean(values[1:]), sd=statistics.stdev(values[1:]))
summary = dict(rows=rows, records=records, events=result['events'], jets=result['jets'], selected_jets=result['selected_jets'], discarded_run=1, histogram_identical_across_modes_and_runs=True, scope='Wall time includes process startup/imports, ROOT reading, array selection, Hist filling, PNG and histogram JSON output; environment setup excluded. No correction backend imported. No cache flushing.')
(campaign/'summary.json').write_text(json.dumps(summary, indent=2))
shutil.copytree(campaign, windows/'c-nice/stored-timing')
doc = windows/'correctionlib-interface-handoff.md'
text = doc.read_text()
anchor = '| C-nice: array nominal file-to-Hist plot only | 8.05 | 0.06 | Remeasured; smaller scope |'
assert anchor in text
text = text.replace(anchor, anchor + f'\n| Stored PHYSLITE pT/eta to Hist; no calibration | {rows["minimal"]["mean"]:.2f} | {rows["minimal"]["sd"]:.2f} | Two input branches |\n| Stored PHYSLITE to Hist; also read C-nice inputs | {rows["extra-inputs"]["mean"]:.2f} | {rows["extra-inputs"]["sd"]:.2f} | I/O control; no calibration |')
text += '\n### Stored-momentum read-and-plot baseline\n\n'
text += 'The standalone `c-nice/plot-stored-jets.py` reads `AnalysisJetsAuxDyn.pt` and `.eta`, uses their stored values without applying any correction, selects pT > 20 GeV and |eta| < 2.5, and fills the same 50-bin, 0–500 GeV Hist plot. It imports no correction backend and loads no payload. All event/jet work uses arrays inside a 50 MB chunk loop. This is a read/plot baseline, not implementation of stored-momentum uncertainty evaluation in the facade. The stored momenta may differ from the recalibrated momenta: equality with the C-nice recalibrated histogram is not assumed.\n\n'
text += 'The second mode reads the original 14 C-nice input branches in addition to stored pT/eta, but does not prepare calibration features or evaluate corrections. It measures the cost of reading more branches; its input set contains two more branches than the recalibration example. Modes ran alternately, four times each, with the first run of each excluded. Both modes and all repetitions yield identical histogram counts, including underflow/overflow, with 20,000 events and 196,606 jets. Timers include startup/imports, reading, selection, histogram filling, PNG output and the small count-report JSON; setup is excluded. No cache flushing was performed. The earlier 8.05-second recalibration result is from a separate campaign. Results and logs: `c-nice/stored-timing/summary.json`.\n'
doc.write_text(text)
(base/'interface-and-results.md').write_text(text)
shutil.copy2(Path(__file__), base/'benchmark-stored-jets.py')
print(json.dumps(summary, indent=2), flush=True)
