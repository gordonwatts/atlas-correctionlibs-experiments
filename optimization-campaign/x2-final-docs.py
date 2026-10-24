from pathlib import Path
import json,shutil,subprocess,statistics
r=Path('/home/gwatts/atlas/correctionlib_tests')
w=Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib')
rows=json.loads((w/'optimization-results/x2-index.json').read_text())
chosen=next(x for x in rows if x['label']=='20-flat-adapters')
fresh=rows[0]['mean'];old=json.loads((w/'optimization-results/11-cached-gaussian/summary.json').read_text())['mean']
b=json.loads((r/'optimization-campaign/x2-B-integration/summary.json').read_text());assert b['passed']
mean=chosen['mean'];sd=chosen['sd']
intro=f'''The selected full-systematics workflow takes **{mean:.2f} ± {sd:.2f} s** on one CPU for 20,000 events (196,606 jets), down from **49.63 ± 0.39 s**: **{old/mean:.2f}× faster**, or **{100*(1-mean/old):.1f}% less time**. It remains {mean-old/2:.2f} s above the 24.81 s target. Against the freshly rebuilt unchanged control ({fresh:.2f} s), the speedup is **{fresh/mean:.2f}×**. No numerical approximation, seed change, tolerance change, systematic reduction, or dropped output/check was used.

The selected code is checkpoint 20. The larger-chunk experiment at checkpoint 21 showed no useful improvement and was reverted; its results remain preserved. The exact JVT cache at checkpoint 18 has no separately established speedup: its apparent gain is smaller than the observed timing variation.
'''
lines=['# Exact-result single-core optimization toward 2×','',intro,'Three runs per checkpoint; discard the first and report mean/sample SD of the remaining two. Runs were sequential, not interleaved, with no forced OS-cache flush. These are short checkpoint measurements on one frozen sample, not precision estimates of production throughput. One-CPU affinity, one native worker and one BLAS/OpenMP thread were enforced. Setup and builds are outside the timer; Python startup, nominal recalibration, all systematic/JVT processing, input/output and every A reference comparison are inside. Nominal-only plots and no-calibration timing were not rerun.','','| Step | Seconds | Change and source | Authoritative WSL commit |','|---|---:|---|---|']
for row in rows:
 link=row['code'] if row['code'].startswith('vendor-patches/') else 'columnar-source/'+row['code']
 suffix=' **Reverted: no useful gain.**' if row['label'].startswith('21-') else (' **Gain inconclusive.**' if row['label'].startswith('18-') else '')
 lines.append(f"| {row['label']} | {row['mean']:.2f} ± {row['sd']:.2f} | {row['description']}{suffix} [Source]({link}) | `{row['commit']}` |")
table='\n'.join(lines[4:])
phases={name:statistics.mean(json.loads((w/f'optimization-results/20-flat-adapters/run-{i}/stage-timing.json').read_text())[name] for i in (2,3)) for name in ('nominal','systematics','jvt')}
lines+=['','## What changed','',
'The earlier profile identified table evaluation as the dominant remaining native work. The first change removes a copy of the non-uniform bin-edge vector on every lookup. The original full-precision JSON patch was preserved separately before rebuilding. The 49.63-to-47.13-second control shift is not credited to the bin-copy fix. The library was built with the same Release settings before and after that fix; exact external-source commits and installed library SHA-256 hashes are retained in each checkpoint’s implementation metadata.',
'',
'The shared backend now binds immutable correction objects during construction, avoiding string construction and map lookup for each jet. Grouped C evaluation computes the common uncertainty once for related up/down and MC/PseudoData states. B scalar evaluation and C grouped evaluation call the same arithmetic and smearing functions. Contiguous adapters reuse native input storage and write results directly into NumPy buffers, avoiding millions of small intermediate allocations. Native scalar loops remain; Python input preparation and event reductions remain array operations.',
'',
'The diagnostic runner now executes nominal, systematic and JVT stages in one process, sharing loaded objects. Each original stage can still run independently. It still writes and rereads the saved kinematics and checks provenance, event identities, jet ordering and statuses. Reference reads are grouped into blocks of 16 branches; the same reference values and tolerances are used. The bounded JVT cache holds one exact sigma-zero result and input copies; changes to effective inputs or tool objects miss the cache, and returned arrays cannot mutate cached data.',
'',
'The C-nice facade remains nominal-only and its call is unchanged. Full-systematics timing composes it with the C diagnostic stages; this does not add a new systematic facade or use Path D. B configuration is unchanged. As before, this diagnostic workflow explicitly reruns nominal calibration for its staged A comparison. Normal PHYSLITE analysis can use stored nominal jets; this is not a claim that nominal recalibration is required for normal analysis.',
'',
'Keeping arrays and tools alive trades memory for startup/read savings. The JVT anchor is proportional to one batch and the reference reader retains one 16-branch block. Grouped native output holds at most four states at once; temporary per-jet vectors have been removed from the array adapters. Peak resident memory was not measured, and multiplicity-dependent truth matching remains a memory consideration. The selected input chunk size is still 1,000 events.',
'', '## Validation and remaining cost','',
f'Every repetition passed the full A nominal, systematic and JVT comparisons. Final raw-byte comparisons against checkpoint 11 passed for all prepared input arrays, nominal stages, 117 kinematic states and 119 JVT/weight states, including dtype, event/jet identities and statuses. Only the intentionally changed backend-provenance metadata is excluded. Evidence: [final parity](optimization-results/x2-checks/11-cached-gaussian-to-20-flat-adapters-parity.json).',
'',
'The existing six backend tests passed after each native build. The grouped API was checked against scalar evaluation across all 117 states, including empty/strided/malformed inputs and invalid groups. Dense adapters were checked against legacy bindings, including nominal stages. JVT-cache checks covered changed inputs, mutation, sigma shifts, replacement tools and empty batches. The initial integrated-runner attempt failed because counts initialization was accidentally conditional on disk loading; this was fixed before acceptance, and its failed logs remain under `16-integrated-workflow`.',
'',
f'Final B integration passed all 20,000 events against A with unchanged `config/reference.yaml` (SHA-256 `{b["config_sha256"]}`). The one-core B run took **{b["wall_seconds"]:.2f} s**. This is a single integration measurement, not a repeated B speedup estimate. Process mappings confirm the rebuilt overlay and expected CorrectionLib library were loaded. See [B summary](optimization-results/x2-B-integration/summary.json) and [loaded libraries](optimization-results/x2-B-integration/loaded-libraries.txt).',
'',
f'The selected uninstrumented stage means are nominal **{phases["nominal"]:.2f} s**, systematics **{phases["systematics"]:.2f} s**, and JVT **{phases["jvt"]:.2f} s**, plus process startup/teardown. Each stage includes its own I/O and validation, so these are not pure kernel times. A further 2× claim needs another {100*(1-(old/2)/mean):.1f}% reduction from the selected result. Before accepting approximations, the next useful experiment is a fresh profile separating JVT computation from reading/writing/reference comparison, under matched production-like output requirements.',
'',
'All implementation checkpoints, the reverted experiment, verification evidence and reports are preserved in local Git. The authoritative source/build is WSL `atlas_al9:/home/gwatts/atlas/correctionlib_tests`; Windows contains synchronized inspection sources and lightweight results. Raw generated ROOT files remain in their original WSL campaign folders. The external CorrectionLib change is also committed in its own repository and exported as patches. Nothing was pushed. The timer summaries record the pre-change HEAD; the table gives the candidate commit containing the implementation and its measurements.','']
report='\n'.join(lines)
(w/'x2-optimization.md').write_text(report)
p=w/'correctionlib-interface-handoff.md';s=p.read_text();s+='\n\n## Exact-result optimization toward 2×\n\n'+intro+'\n'+table+'\n\nThe implementation changes and validation details are in [the complete campaign report](x2-optimization.md). B passed again with its original configuration; all final C physics buffers match checkpoint 11 byte for byte. All changes are local commits; nothing was pushed.\n';p.write_text(s)
for name in ('c-nice-optimization.md','single-thread-profiling.md','four-path-timing-comparison.md','c-nice/README.md'):
 p=w/name;s=p.read_text();s=s.replace('Current full-systematics single-core timing is **49.63 ± 0.39 s**.','The earlier full-systematics single-core timing was **49.63 ± 0.39 s**.')
 link='../x2-optimization.md' if name.startswith('c-nice/') else 'x2-optimization.md'
 s+=f'\n\nThe subsequent exact-result single-core campaign selected **{mean:.2f} ± {sd:.2f} s** ({old/mean:.2f}× versus 49.63 s; {fresh/mean:.2f}× versus its rebuilt control). See [the per-change table and acceptance report]({link}). All A comparisons and outputs remain enabled, final C arrays match the prior checkpoint byte for byte, and B passed with unchanged configuration. The 5,000-event chunk experiment was reverted; 1,000-event chunks remain selected.\n';p.write_text(s)
for name in ('x2-optimization.md','correctionlib-interface-handoff.md','c-nice-optimization.md','single-thread-profiling.md','four-path-timing-comparison.md','c-nice/README.md'):
 (r/name).write_text((w/name).read_text().replace('columnar-source/','').replace('optimization-results/','optimization-campaign/'))
shutil.copy2(w/'x2-final-docs.py',r/'optimization-campaign/x2-final-docs.py')
subprocess.run(['git','add','x2-optimization.md','correctionlib-interface-handoff.md','c-nice-optimization.md','single-thread-profiling.md','four-path-timing-comparison.md','c-nice/README.md','optimization-campaign/x2-final-docs.py'],cwd=r,check=True)
subprocess.run(['git','commit','-q','-m','Document 1.88x exact-result single-core campaign, individual commits and final B acceptance'],cwd=r,check=True)
print(json.dumps({'selected_mean':mean,'speedup_old':old/mean,'speedup_control':fresh/mean,'B_seconds':b['wall_seconds'],'phases':phases},indent=2))


