from pathlib import Path
import json,shutil,subprocess,statistics,re
r=Path('/home/gwatts/atlas/correctionlib_tests');w=Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib')
b=json.loads((r/'optimization-campaign/nn-B-integration/summary.json').read_text());assert b['passed']
assert b['config_sha256']=='4d84aa237b6a1796c05e9a38fa341e89993f1b8c582bc99da7b1624b4ed46ca8'
for folder in ('nn-checks','nn-B-integration'):
 dest=w/'optimization-results'/folder;dest.mkdir(exist_ok=True)
 for p in (r/'optimization-campaign'/folder).iterdir():
  if p.is_file() and p.suffix in ('.json','.log','.txt'):shutil.copy2(p,dest/p.name)
for p in (r/'optimization-campaign/x2-checks').glob('*-to-2[56]-*-parity.json'):shutil.copy2(p,w/'optimization-results/x2-checks'/p.name)
for name in ('check-nn-b.py','check-nn-b.sh','finalize-nn-work.py'):
 shutil.copy2(w/name,r/'optimization-campaign'/name)
subprocess.run(['git','add','optimization-campaign'],cwd=r,check=True)
subprocess.run(['git','commit','-q','-m','Validate shared NN optimizations in B and preserve exact-output acceptance evidence'],cwd=r,check=True)
rows=json.loads((w/'optimization-results/x2-index.json').read_text());current=next(x for x in rows if x['label'].startswith('26-'));before=next(x for x in rows if x['label'].startswith('20-'));cache=next(x for x in rows if x['label'].startswith('25-'))
mean=current['mean'];sd=current['sd'];baseline=json.loads((w/'optimization-results/11-cached-gaussian/summary.json').read_text())['mean']
summary=f'**Selected full-systematics result: {mean:.2f} ± {sd:.2f} s on one CPU.** Both NN changes are implemented in the shared B/C inference backend. This is {baseline/mean:.2f}× faster than the 49.63 s checkpoint and {before["mean"]-mean:.2f} s below the pre-NN 26.44 s checkpoint. These are short sequential measurements, not a controlled estimate of each change’s isolated gain.'
report=f'''# Shared NN feature caching and reusable buffers

{summary}

| Checkpoint | Full C-nice seconds | WSL implementation commit |
|---|---:|---|
| Before these NN changes (20) | {before['mean']:.2f} ± {before['sd']:.2f} | `{before['commit']}` |
| Exact feature cache (25) | {cache['mean']:.2f} ± {cache['sd']:.2f} | `{cache['commit']}` |
| Cache plus reusable buffers (26) | {mean:.2f} ± {sd:.2f} | `{current['commit']}` |

Three runs per checkpoint; discard the first and report mean/sample SD of the other two. Same 20,000 events, 196,606 jets, nominal recalibration, all 117 kinematic and 119 JVT/weight states, outputs and A comparisons. Input chunks remain 1,000 events, with one-CPU affinity and one native worker. The cache alone did not demonstrate a full-workflow gain. Buffer reuse reduced the measured mean, but the small sample and variation limit precise attribution. No precision or acceptance-tolerance changes were made.

## Implementation

[Shared inference wrapper](columnar-source/src/nn_graph.hxx): exact feature memoization uses all four double input bit patterns, after the existing caller’s float rounding. A model-specific identity prevents reuse across payloads or object lifetimes. Entries are thread-local and bounded to 262,144 slots, approximately 12 MiB per thread. Each slot retains its first tuple, favoring nominal features; a collision evaluates normally. Non-finite inputs bypass the cache. This is feature-keyed rather than keyed by jet position, so it works for B’s scalar traversal and C’s array batches without depending on chunk size, ordering or jet identity. Equal features from another jet are safe to reuse because inference depends only on these features and the immutable model. Selection, validity and weights are still evaluated for the current variation.

[Reusable buffers](columnar-source/src/nn_buffered.hxx): a conservative execution plan handles the supported single-input dense chain. It reuses thread-local Eigen input/intermediate vectors, preserves double arithmetic, and keeps matrix multiplication, bias, ReLU and sigmoid operations separate. Input reordering, normalization and sigmoid clipping follow lwtnn. Unsupported graph forms continue through the original FastGraph implementation. No graph rewrite, reduced precision or activation approximation is used.

C’s [NN evaluator](columnar-source/src/nnjvt.hxx) calls this backend. B previously used the stock ATLAS NN tagger for inference; its [tagger overlay](columnar-source/atlas-overlay/JetMomentTools/Root/JetVertexNNTagger.cxx) now uses the same backend. The overlay is based on the pinned AnalysisBase 25.2.100 JetMomentTools package. Configuration, feature preparation and jet-selection logic remain unchanged. The C-nice public interface is unchanged.

## Acceptance

- Each checkpoint passed every full-sample A comparison in all three timing runs.
- Saved inputs, nominal stages and every kinematic/JVT output buffer match checkpoint 20 byte for byte. Checkpoint 26 also matches checkpoint 25. Dtypes, IDs and statuses are included; intentionally changed provenance metadata is excluded.
- 302,104 exact score comparisons passed against the original lwtnn calculation, covering repeats, cache collisions, reverse traversal, thread isolation, model switching and extreme/signed-zero inputs.
- Buffer checks confirmed the optimized path was active, performed no Eigen heap allocations after warmup, preserved reordered input semantics and fell back correctly for unsupported activation. A staging-header include in an early test was corrected; acceptance uses the authoritative WSL header and the stronger active-path test.
- The existing six backend tests passed after each native build.
- B passed the complete 20,000-event comparison with A. Its one-core integration run took **{b['wall_seconds']:.2f} s**; this is one validation run, not a repeated B performance estimate. Process mappings confirm both the new `libJetMomentToolsLib.so` and the other shared overlay libraries were loaded. Configuration SHA-256 remains `{b['config_sha256']}`.

The [complete timing history](x2-optimization.md) now includes the original optimization baseline, all intermediate checkpoints, the four-thread interval, reverted experiments, these NN steps, earlier A/B/C/D comparisons, nominal plotting and no-calibration baselines. These scopes are labeled separately.

Evidence: [B summary](optimization-results/nn-B-integration/summary.json), [loaded libraries](optimization-results/nn-B-integration/loaded-libraries.txt), [buffer tests](optimization-results/nn-checks/check-26-buffer-edges.log), and [full output parity](optimization-results/x2-checks/20-flat-adapters-to-26-nn-reusable-buffers-parity.json).

All changes, checks and documentation are committed locally in both repositories. Nothing was pushed.
'''
(w/'nn-optimization.md').write_text(report)
p=w/'x2-optimization.md';s=p.read_text();start=s.index('The selected full-systematics workflow');end=s.index('The table includes every optimization checkpoint',start)
s=s[:start]+summary+'\n\nCheckpoint 26 is selected. Checkpoint 21’s larger input chunks were reverted. Checkpoints 18 and 25 have no individually established performance gain. See [the NN implementation and acceptance report](nn-optimization.md).\n\n'+s[end:]
# Keep prior campaign narrative explicitly historical, avoiding stale current-result claims.
s=s.replace('## What changed','## Earlier campaign through checkpoint 20').replace('## Validation and remaining cost','## Validation and remaining cost at checkpoint 20').replace('Final B integration passed','B integration at checkpoint 20 passed')
s=s.replace('The selected uninstrumented stage means','The checkpoint-20 uninstrumented stage means')
s=s.replace('A further 2× claim needs another','At checkpoint 20, a further 2× claim needed another')
s=s.replace('Cache exact NN input tuples in a bounded model-scoped native cache shared by B and C [Source]', 'Cache exact NN input tuples in a bounded model-scoped native cache shared by B and C. **Gain inconclusive.** [Source]')
handoff=(w/'correctionlib-interface-handoff.md').read_text();a=handoff.index('| Path / workflow |');z=handoff.index('\n\n',a);historical=handoff[a:z]
initial=json.loads((w/'c-nice/summary.json').read_text());plot_sd=statistics.stdev(initial['plot_runs_seconds'][1:])
historical += f'\n| Initial C-nice before array update: full workflow | {initial["workflow_mean"]:.2f} | {initial["workflow_sd"]:.2f} | Earlier campaign |\n| Initial C-nice before array update: nominal plot | {initial["plot_mean"]:.2f} | {plot_sd:.2f} | Earlier campaign |\n| Original C: full workflow | 167.70 | 0.95 | Historical four-path campaign |\n| D: full workflow | 283.21 | 22.37 | Historical four-path campaign |'
s+='\n\n## Earlier cross-path, plotting and no-calibration timings\n\nThese are historical measurements with different scopes; none was remeasured for the NN changes. A/B full-job reference comparisons are outside their timers, while C/C-nice/D diagnostic comparisons are inside. Nominal plotting reruns nominal calibration. Stored-jet rows bypass calibration; the A stored-jet plot uses an intermediate tree. Do not treat these rows as matched reader or facade-overhead benchmarks.\n\n'+historical+'\n\n## B integration timings after backend changes\n\nThese are individual validation runs, not repeated benchmark means.\n\n| After checkpoint | B wall seconds | Validation |\n|---|---:|---|\n| 03 | 288.82 | Full A comparison passed |\n| 11 | 281.61 | Full A comparison passed |\n| 20 | 271.04 | Full A comparison passed |\n'+f'| 26 | {b["wall_seconds"]:.2f} | Full A comparison passed; optimized NN tagger loaded |\n'
p.write_text(s)
for name in ('c-nice-optimization.md','four-path-timing-comparison.md'):
 p=w/name;s=p.read_text();s=re.sub(r'\*\*(?:Current|Selected) single-threaded result:.*?\n\n',summary+' See the [complete timing history](x2-optimization.md).\n\n',s,count=1,flags=re.S);p.write_text(s)
for name in ('correctionlib-interface-handoff.md','c-nice/README.md'):
 p=w/name;link='../nn-optimization.md' if name.startswith('c-nice/') else 'nn-optimization.md';s=p.read_text();s+='\n\n## Shared NN optimization update\n\n'+summary+f' Full details and validation: [NN optimization report]({link}).\n';p.write_text(s)
for name in ('nn-optimization.md','x2-optimization.md','c-nice-optimization.md','four-path-timing-comparison.md','correctionlib-interface-handoff.md','c-nice/README.md'):
 (r/name).write_text((w/name).read_text().replace('columnar-source/','').replace('optimization-results/','optimization-campaign/'))
subprocess.run(['git','add','nn-optimization.md','x2-optimization.md','c-nice-optimization.md','four-path-timing-comparison.md','correctionlib-interface-handoff.md','c-nice/README.md'],cwd=r,check=True)
subprocess.run(['git','commit','-q','-m','Document complete timing history and shared NN cache/buffer acceptance'],cwd=r,check=True)
print(json.dumps({'selected_seconds':mean,'speedup_vs_49_63':baseline/mean,'B_seconds':b['wall_seconds']},indent=2))

