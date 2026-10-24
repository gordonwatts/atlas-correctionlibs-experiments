from pathlib import Path
import hashlib,json,shutil,subprocess
root=Path('/home/gwatts/atlas/correctionlib_tests')
win=Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib')
base=root/'optimization-campaign/29-uniform-binning'
new=json.loads((base/'summary.json').read_text());old=json.loads((base/'control/summary.json').read_text())
conversion=json.loads((base/'conversion.json').read_text())
audit=json.loads((base/'axis-audit.json').read_text())
b=json.loads((base/'B-integration/summary.json').read_text())
assert new['passed'] and old['passed'] and b['passed']
parity=root/'optimization-campaign/x2-checks/26-nn-reusable-buffers-to-29-uniform-binning-parity.json'
assert parity.exists();shutil.copy2(parity,base/'parity.json')
commit=subprocess.check_output(['git','log','-1','--format=%H','--','python/atlas_correctionlib/nodes.py'],cwd=root,text=True).strip()
manifest=json.loads((root/'payloads/bundle-manifest.json').read_text())
for name,item in conversion['payloads'].items():
    digest=hashlib.sha256((root/'payloads'/name).read_bytes()).hexdigest()
    assert item['after_sha256']==digest==manifest['files'][name]['sha256']
checks=dict(implementation_commit=commit,conversion_nodes=len(conversion['changes']),B=b,control_mean=old['mean'],control_sd=old['sd'],mean=new['mean'],sd=new['sd'],no_active_lookup_changed=True,performance_gain_established=False)
(base/'acceptance.json').write_text(json.dumps(checks,indent=2)+'\n')
lines=['# Conservative uniform-bin encoding — checkpoint 29','',
 f'Uniform encoding is enabled where the installed CorrectionLib arithmetic preserves finite-input bin membership. The updated C-nice measurement is **{new["mean"]:.2f} ± {new["sd"]:.2f} s**, compared with a fresh unchanged control of **{old["mean"]:.2f} ± {old["sd"]:.2f} s**. **No speedup is established:** only 12 inactive lookup nodes qualified for conversion in this payload.','',
 '| Full-systematics C-nice | First run, excluded | Run 2 | Run 3 | Mean ± sample SD |','|---|---:|---:|---:|---:|']
for label,s in [('Fresh checkpoint-26 control',old),('Checkpoint 29: uniform encoding',new)]:
    times=[r['wall_seconds'] for r in s['records']]
    lines.append(f'| {label} | {times[0]:.3f} | {times[1]:.3f} | {times[2]:.3f} | {s["mean"]:.3f} ± {s["sd"]:.3f} |')
lines += ['',
 'Same 20,000 events, 196,606 jets, 1,000-event chunks, one CPU and one native/BLAS/OpenMP worker. Startup, nominal recalibration, full systematic/JVT processing, outputs and all A comparisons remain timed. Three sequential repetitions per version; discard the first. The control precedes the candidate, without cache flushing or interleaving. Earlier checkpoint 26 remains recorded at 25.61 ± 0.31 s; use the fresh control when assessing this change.','',
 '## Why the conversion is limited','',
 'Equal spacing does not guarantee equivalent bin selection with the installed evaluator. Explicit edges use `upper_bound`; uniform edges use `n * ((x-low)/(high-low))` followed by integer conversion. Subtraction, division or multiplication can round a value immediately below an edge onto the edge. For example, with edges `[-4.4, 0, 4.4]`, the representable double immediately below zero belongs to bin 0 in the explicit representation but the uniform arithmetic selects bin 1. Some axes also round an in-range point just below the upper limit to index n.','',
 '| Audit classification | Distinct axes | Binning nodes |','|---|---:|---:|']
for reason,item in audit['summary'].items():lines.append(f'| {reason} | {item["axes"]} | {item["nodes"]} |')
lines += ['',
 'The one eligible axis is `[15, 7000]`, a single bin. Its 12 occurrences are the non-interpolated (`None`) gluon-fraction and gluon-fraction-error tables for six jet definitions. The selected systematic backend uses the `Full` interpolation versions, so its active lookup paths are unchanged. Nominal and JVT-efficiency payload paths receive no uniform replacement. In particular, this experiment has not accelerated the hot interpolated tables from the preceding lookup study.','',
 '## Implementation and validation','',
 '- The shared export helper checks approximate equal spacing, then checks each exact edge and its adjacent representable doubles using the installed evaluator arithmetic order. Monotonic indexing between those transitions provides the finite-input bin-membership criterion. Axes with infinite edges, nonuniform spacing or unsafe transitions remain explicit. Wrap flow also remains explicit. This criterion depends on the current arithmetic implementation and should be revalidated after evaluator/compiler changes.',
 '- Eligible axes emit schema-v2 `{n, low, high}` edges. The change is shared by both nominal and uncertainty exporters. Current payloads were updated by replacing only certified edge representations, preserving contents, formulas, names and source constants. The runtime bundle manifest was updated to match compressed payload hashes. Original payloads remain in the campaign directory and Git history.',
 '- Native CorrectionLib tests compare exact bin-index outputs for 100,010 edge, neighboring-value, random and overflow points for each of clamp and default-value flow, plus error-flow boundary checks. The eligibility guard rejects the known unsafe axis and nonuniform/infinite axes. NaN inputs are outside this finite-input equivalence claim.',
 '- Existing six backend tests passed. All three updated C-nice runs passed the full A comparisons. Saved inputs, nominal outputs, all 117 kinematic states and 119 JVT/weight states match checkpoint 26 byte for byte, excluding changed provenance metadata.',
 f'- B passed the full 20,000-event A comparison with unchanged configuration; its single validation run took **{b["wall_seconds"]:.2f} s**. Loaded-library records confirm the expected overlays and CorrectionLib library. This is a single integration timing, not a repeated B speed estimate.',
 '- No C++ implementation, public C-nice interface, B configuration, seed or precision setting changed.', '',
 f'Implementation commit: `{commit}`. [Shared exporter](columnar-source/python/atlas_correctionlib/nodes.py), [native validation/conversion](uniform-binning/validate_and_convert.py), [conversion manifest](optimization-results/29-uniform-binning/conversion.json), [output parity](optimization-results/29-uniform-binning/parity.json), [acceptance](optimization-results/29-uniform-binning/acceptance.json).','',
 'A broader speed improvement would require a separate change to uniform indexing or payload structure with explicit boundary validation. Simply classifying approximately equally spaced edges as uniform would change some bin decisions.','',
 'All changes and evidence are committed locally in the authoritative WSL and Windows inspection repositories. Nothing was pushed.','']
text='\n'.join(lines)
(win/'uniform-binning.md').write_text(text)
(root/'uniform-binning.md').write_text(text.replace('columnar-source/','').replace('optimization-results/','optimization-campaign/'))
p=win/'x2-optimization.md';text=p.read_text()
start=text.index('| Step |');end=text.index('\n\n',start)
assert '29-uniform-binning |' not in text
row=f'| 29-uniform-binning | {new["mean"]:.2f} ± {new["sd"]:.2f} | Conservative uniform encoding; 12 inactive nodes eligible, **no demonstrated speedup**. Fresh control {old["mean"]:.2f} ± {old["sd"]:.2f} s. [Source](columnar-source/python/atlas_correctionlib/nodes.py) | `{commit}` |'
text=text[:end]+'\n'+row+text[end:]
text=text.replace('Checkpoint 26 is selected.','Checkpoint 29 is selected; its conservative uniform encoding does not change active lookup paths. [Study and fresh control](uniform-binning.md).')
first=text.index('**Selected full-systematics result:');last=text.index('\n\n',first)
text=text[:first]+f'**Latest full-systematics result: {new["mean"]:.2f} ± {new["sd"]:.2f} s on one CPU.** The uniform-bin step has no demonstrated speedup over its fresh unchanged control ({old["mean"]:.2f} ± {old["sd"]:.2f} s). The preceding NN checkpoint measured 25.61 ± 0.31 s. These short sequential measurements are not isolated estimates of each change’s gain.'+text[last:]
text+='\n\n## Uniform encoding follow-up (24 October 2026)\n\nSee [the uniform-bin study](uniform-binning.md) for the complete axis audit, boundary checks, fresh control timing and B acceptance. Checkpoint 29 is a conservative representation change; none of the active lookup nodes qualified.\n'
p.write_text(text);(root/p.name).write_text(text.replace('columnar-source/','').replace('optimization-results/','optimization-campaign/'))
dest=win/'optimization-results/29-uniform-binning';dest.mkdir(exist_ok=False)
for source in base.rglob('*'):
    if source.is_file() and source.suffix in ('.json','.log','.txt','.py') and not any(part.startswith('workDir') for part in source.parts):
        target=dest/source.relative_to(base);target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
shutil.copy2(root/'python/atlas_correctionlib/nodes.py',win/'columnar-source/python/atlas_correctionlib/nodes.py')
print('\n'.join(lines[:10]));print('B',b['wall_seconds'],flush=True)
