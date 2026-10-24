from pathlib import Path
import hashlib,json,statistics,shutil,subprocess
root=Path('/home/gwatts/atlas/correctionlib_tests')
win=Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib')
out=root/'optimization-campaign/28-lookup-comparison-v2'
raw=json.loads((out/'raw.json').read_text());jvt=json.loads((out/'jvt.json').read_text())
assert raw['passed'] and jvt['passed']
maps=(out/'loaded-libraries.txt').read_text()
assert 'build-overlay' not in maps
atlas='/cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt'
assert atlas+'/lib/libJetUncertaintiesLib.so' in maps
lib=root/'venv/lib/python3.13/site-packages/correctionlib/lib/libcorrectionlib.so'
assert str(lib) in maps
meta=json.loads((out/'metadata.json').read_text())
for name,h in meta['hashes'].items():assert hashlib.sha256((root/name).read_bytes()).hexdigest()==h
provenance=dict(benchmark_commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip(),library_hashes={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in [Path(atlas)/'lib/libJetUncertaintiesLib.so',lib]},benchmark_hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (root/'lookup-benchmark').glob('*') if p.is_file()},production_hashes_unchanged=True)
(out/'acceptance.json').write_text(json.dumps(provenance,indent=2)+'\n')
summary={}
lines=['# Original ATLAS versus CorrectionLib lookup timing','',
 'On this sample, the current CorrectionLib lookup path is slower than the original ATLAS/ROOT lookup. Reusing CorrectionLib argument vectors closes part of the gap; it is a benchmark-only experiment here. The full shared systematic replay differs by about two tenths of a second. No production backend or payload was changed.','',
 '## Matched lookup measurements','',
 'Median nanoseconds per lookup (JVT rows are per content/error pair), excluding the first of seven rounds. Every retained round rotates the order of the three implementations. All data and objects are loaded before timing; table names are resolved once.','',
 '| Lookup | Tables | ATLAS/ROOT ns | CorrectionLib ns | CorrectionLib with reused arguments ns | Current / ATLAS |',
 '|---|---:|---:|---:|---:|---:|']
items=[(name,item,item['lookups_per_sweep'],item['tables']) for name,item in raw['lookup_categories'].items()]
items += [(name+' content + error',jvt[name],jvt[name]['pairs_per_sweep'],1) for name in ['JvtFixedEffPt','EffFixedEffPt']]
for name,item,count,nt in items:
    stats={key:dict(median_ns=statistics.median(r[key] for r in item['rounds'][1:])*1e9/count,mean_seconds=statistics.mean(r[key] for r in item['rounds'][1:]),sample_sd_seconds=statistics.stdev(r[key] for r in item['rounds'][1:])) for key in item['rounds'][0]}
    a,c,r=[stats[k]['median_ns'] for k in ['atlas','correctionlib','correctionlib_reused_arguments']]
    summary[name]=stats
    lines.append(f'| {name} | {nt} | {a:.2f} | {c:.2f} | {r:.2f} | {c/a:.2f}× |')
systematics=raw['shared_systematics']
sysstats={k:dict(mean=statistics.mean(r[k] for r in systematics['rounds'][1:]),sd=statistics.stdev(r[k] for r in systematics['rounds'][1:]),median=statistics.median(r[k] for r in systematics['rounds'][1:])) for k in ['atlas','correctionlib']}
summary['shared_systematics']=sysstats
a,c=sysstats['atlas'],sysstats['correctionlib']
lines += ['',
 'The interpolation benchmark covers all 58 distinct table/mode objects bound by the selected systematic evaluator. Each table is swept over all 196,606 nominal jets; this microbenchmark uses pT in GeV and absolute eta and weights each table equally. It is not the production frequency distribution. The shared-evaluator test below uses the original recipe-dependent signed/absolute eta, conditions, component combinations and call frequencies.','',
 'The original uncertainty implementation is the stock AnalysisBase 25.2.100 `jet::UncertaintyHistogram`, loaded from CVMFS, including its boundary handling, interpolation and cached projections. CorrectionLib uses the existing exported tables and the installed full-precision/bin-edge-reference-patched library. The reused-argument column still executes the same CorrectionLib graph; it only avoids creating the outer argument vector for every call. It is not a production optimization result.','',
 'For JVT, the benchmark reproduces the original `JvtEfficiencyToolBase.cxx` helper: find the two bins once, then retrieve content and error. CorrectionLib uses two named corrections, so it searches twice. Both receive the same 58,777 valid nominal-jet points, repeated 64 times per sweep, and return float content/error values. Input filtering happens before timing; ROOT retains its helper validity checks, while CorrectionLib is given prevalidated inputs. This is a lookup-pair comparison, not the complete efficiency weighting algorithm.','',
 '## Swap only the lookup in our shared systematic evaluator','',
 f'A test-only copy of `systematics.hxx` changes `Table::operator()` to select the original ATLAS histogram or the existing CorrectionLib correction. Everything else uses the current shared backend, including its grouped API, arithmetic, RNG and exact Gaussian cache. Both paths are warmed before timing. The replay processes all {raw["jets"]:,} jets, {systematics["groups"]} groups and {systematics["states"]} output states.','',
 '| Lookup used by shared backend | Mean ± sample SD | Median |','|---|---:|---:|',
 f'| Original ATLAS | {a["mean"]:.4f} ± {a["sd"]:.4f} s | {a["median"]:.4f} s |',
 f'| CorrectionLib | {c["mean"]:.4f} ± {c["sd"]:.4f} s | {c["median"]:.4f} s |','',
 f'CorrectionLib adds **{c["mean"]-a["mean"]:.3f} s** in the mean of this replay (**{100*(c["mean"]/a["mean"]-1):.1f}%** relative to the ATLAS lookup version). Six retained alternating-order measurements follow one excluded warm-up round. This includes native input-row copying, result writes and shared physics; excludes Python, file reading, tool loading, validation, nominal calibration and JVT. Preallocated output is overwritten group by group. The additional runtime lookup-selection branch is present in both alternatives. This is not a measurement of Path A or B as a whole.','',
 '## Numerical checks and provenance','',
 f'- {sum(x["sample_validation"]["count"] for x in raw["lookup_categories"].values()):,} direct uncertainty lookups and {sum(x["boundary_validation"]["count"] for x in raw["lookup_categories"].values()):,} edge/nextafter checks passed at absolute tolerance 1e-10 plus relative tolerance 1e-9. The 2D interpolation has tiny rounding differences (largest sample absolute difference 6.66e-16); it is not claimed bit-identical.',
 f'- All {systematics["validation"]["count"]:,} output values in the shared systematic comparison were exactly equal. Existing kinematic tolerance checks also passed. This is equality of the tested numeric values, not a binary-file comparison.',
 '- Both JVT content/error comparisons were exactly equal after the original float conversion. Invalid-input and JVT boundary behavior are outside this timing study; the systematic boundary checks do not cover JVT.',
 '- One CPU (CPU 0), compiled with `-O2 -g -DNDEBUG`, no fast-math. Loading, ROOT projection setup and validation are outside timers. Short cache-warm native microbenchmarks; ratios should not be extrapolated to startup or full-workflow timing.',
 '- Source/payload/input hashes and loaded-library paths were checked. Production source hashes remained unchanged. The original 28-lookup-comparison directory retains the first compile attempt; it needed the standalone AnalysisBase compiler definition. The accepted results are in 28-lookup-comparison-v2.',
 '- Nominal JES/GSC evaluation, NN inference and complete A/B/C workflows were not benchmarked here. No alternative production backend is selected by this study.','',
 'CorrectionLib therefore contributes payload portability and a common evaluator here, rather than an observed lookup speed advantage. The cost depends on representation: generic formula/transform traversal and argument handling add work; separate content/error corrections repeat a bin search that ROOT shares. These mechanisms explain plausible overheads, but this timing study does not individually profile their costs.','',
 'Sources: [benchmark harness](lookup-benchmark/benchmark.cxx), [lookup adapters](lookup-benchmark/lookup.hxx), [JVT comparison](lookup-benchmark/jvt.cxx). Evidence: [raw systematic measurements](optimization-results/28-lookup-comparison-v2/raw.json), [JVT measurements](optimization-results/28-lookup-comparison-v2/jvt.json), [summary](optimization-results/28-lookup-comparison-v2/summary.json), [provenance](optimization-results/28-lookup-comparison-v2/acceptance.json).','',
 'Implementation, validation and this report are committed locally in both repositories. Nothing was pushed.','']
(out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
report='\n'.join(lines)
(win/'lookup-comparison.md').write_text(report)
(root/'lookup-comparison.md').write_text(report.replace('optimization-results/','optimization-campaign/'))
for where,prefix in [(win,'optimization-results'),(root,'optimization-campaign')]:
    path=where/'x2-optimization.md'
    text=path.read_text()
    heading='## Matched lookup comparison (24 October 2026)'
    assert heading not in text
    path.write_text(text.rstrip()+f'\n\n{heading}\n\nA test-only swap inside the shared systematic backend measured {a["mean"]:.3f} ± {a["sd"]:.3f} s with original ATLAS histogram lookups versus {c["mean"]:.3f} ± {c["sd"]:.3f} s with CorrectionLib. All 117 states matched exactly on the replay sample. This is a native calculation study, not a new C-nice optimization checkpoint or end-to-end timing. See [the lookup comparison](lookup-comparison.md) for the direct interpolation and JVT lookup measurements, scope and validation.\n')
dest=win/'optimization-results/28-lookup-comparison-v2';dest.mkdir(exist_ok=False)
for source in out.rglob('*'):
    if source.is_file() and source.suffix in ('.json','.txt','.log','.hxx'):
        target=dest/source.relative_to(out);target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
print(report)
