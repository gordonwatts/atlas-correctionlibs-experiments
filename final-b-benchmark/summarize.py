"""Publish the completed final-B campaign into existing timing reports."""
from pathlib import Path
import json
import shutil

root = Path('/home/gwatts/atlas/correctionlib_tests')
win = Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib')
campaign = root/'optimization-campaign/27-final-B-timing'
summary = json.loads((campaign/'summary.json').read_text())
mean, sd = summary['mean_seconds'], summary['sample_sd_seconds']
assert len(summary['runs']) == 4 and all(r['passed'] for r in summary['runs'])
assert all(r['cpu_affinity'] == summary['runs'][0]['cpu_affinity'] for r in summary['runs'])
assert all(r['config_sha256'] == '4d84aa237b6a1796c05e9a38fa341e89993f1b8c582bc99da7b1624b4ed46ca8' for r in summary['runs'])
reduction = 100*(1-mean/290.70)
lines = [
    '## Final repeated Path B timing (24 October 2026)', '',
    f'The final shared backend takes **{mean:.2f} ± {sd:.2f} s** in Path B, compared with the historical **290.70 ± 1.01 s** baseline: **{reduction:.1f}% less elapsed time**. This comparison shows no observed slowdown, but is not an interleaved regression test. The historical baseline did not enforce the current one-CPU affinity policy.', '',
    '| Measurement | Wall seconds |', '|---|---:|',
    '| Historical B baseline, mean ± sample SD | 290.70 ± 1.01 |',
]
for i, run in enumerate(summary['runs'], 1):
    lines.append(f"| Final B run {i}{' (excluded)' if i == 1 else ''} | {run['wall_seconds']:.2f} |")
lines += [
    f'| **Final B mean ± sample SD, runs 2–4** | **{mean:.2f} ± {sd:.2f}** |', '',
    'Four sequential runs of 20,000 events, full systematics and unchanged `config/reference.yaml`; first run excluded, no forced cache flush. One-CPU affinity and one native/BLAS/OpenMP worker. Environment setup is outside the timer; process startup, tool initialization, computation and ROOT output are inside. The library-observation loop polls every 0.5 s, adding up to roughly 0.5 s of finish-detection delay. All four full A comparisons passed outside the timed jobs. Loaded-library records confirm the rebuilt JetMomentTools and JetUncertainties overlays and expected CorrectionLib library.', '',
    'No physics implementation or configuration changed for this measurement. This updates the end-of-campaign B measurement only; earlier checkpoints were not rerun. Raw ROOT outputs and logs remain in WSL. [Run summaries and provenance](optimization-results/27-final-B-timing/summary.json).', '',
]
section = '\n'.join(lines)
for filename in ('x2-optimization.md', 'four-path-timing-comparison.md', 'nn-optimization.md'):
    p = win/filename
    text = p.read_text()
    assert lines[0] not in text, 'Do not duplicate final benchmark section'
    text = text.rstrip()+'\n\n'+section
    p.write_text(text)
    (root/filename).write_text(text.replace('columnar-source/', '').replace('optimization-results/', 'optimization-campaign/'))
destination = win/'optimization-results/27-final-B-timing'
destination.mkdir(exist_ok=False)
for source in campaign.rglob('*'):
    if source.is_file() and source.suffix in ('.json', '.txt', '.log'):
        target = destination/source.relative_to(campaign)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
print(section)
