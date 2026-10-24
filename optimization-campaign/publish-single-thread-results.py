from pathlib import Path
import json,shutil,re
root=Path('/home/gwatts/atlas/correctionlib_tests')
windows=Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib')
index=json.loads((windows/'optimization-results/index.json').read_text())
rows=[r for r in index if int(r['label'][:2])>=9]
baseline,final=rows[0],rows[-1]
b=json.loads((root/'optimization-campaign/11-B-integration/summary.json').read_text())
native_after=(root/'optimization-campaign/11-profile/callgrind-no.txt').read_text()
def percent(function):
    line=next(line for line in native_after.splitlines() if function in line)
    return float(re.search(r'\(\s*([\d.]+)%\)',line)[1])
lines=['# Single-threaded C-nice and RNG profiling','',
f"Current full-systematics C-nice runtime: **{final['mean']:.2f} ± {final['sd']:.2f} s**, on one CPU. The four-thread result from the previous campaign is historical. The default native worker count is now one. This follow-up reduces the single-core baseline of {baseline['mean']:.2f} s by {100*(1-final['mean']/baseline['mean']):.1f}%.",'',
'Each checkpoint processes the same 20,000 events / 196,606 jets and all 117 kinematic and 119 JVT/weight states. Three sequential runs per checkpoint, first discarded; mean and sample SD use the remaining two. Numerical-library thread limits are one and CPU affinity is restricted to one logical CPU, including child processes. This also prevents incidental reader/decompression workers from contributing parallel CPU throughput. Initialization, nominal recalibration, all outputs and A-reference validation remain inside the timer; setup and compilation remain outside. The earlier four-thread campaign did not use this affinity restriction.','',
'| Change, cumulative | Full workflow seconds | Shared source | WSL implementation commit |','|---|---:|---|---|']
for r in rows:
    lines.append(f"| {r['description']} | {r['mean']:.2f} ± {r['sd']:.2f} | [source](columnar-source/{r['code']}) | `{r['commit']}` |")
lines += ['', '## What the profiles show', '',
'The first optimization campaign used stage timing and targeted comparisons, rather than a function-level profile. This follow-up adds cProfile across the complete three-stage workflow and Callgrind for the shared native systematic evaluator. Native profiling uses 2,048 evenly spaced jets from the frozen sample, all 117 states, and the same compiler toolchain at -O2. Payload initialization is outside the native collection region. Python profiling retains full sample initialization, I/O and validation. Profiled elapsed times include instrumentation overhead and must not be substituted for the uninstrumented timing table.', '',
'Before the RNG changes, the native profile attributed **15.96% of instructions to `TRandom3::SetSeed` itself** and **6.97% to `TRandom3::Rndm` itself**. RNG work is therefore material. The table lookup/evaluation path accounted for **65.73% inclusive** of instructions, including work in its callees. Inclusive percentages overlap; these figures are instruction counts, not wall-time percentages.', '',
f"After the Gaussian-cache change, the same native profile attributes **{percent('TRandom3::SetSeed'):.2f}%** to seeding and **{percent('TRandom3::Rndm'):.2f}%** to random-number generation itself. The original RNG stream is retained, while repeated RNG work is largely removed.", '',
'Profiled systematic-batch time falls from **22.08 s to 13.82 s**. The table lookup/evaluation path now accounts for **85.42% inclusive** of native instructions; its absolute instruction count is essentially unchanged. It is the next substantial native optimization target. The uninstrumented final phase means are approximately 6.15 s nominal, 24.37 s systematic, and 19.11 s JVT, including their I/O and validation.', '',
'The baseline Python profile attributed about **22.08 s** to the 117 native systematic batch calls. Reading A-reference branches took **8.55 s** in the systematic stage and **6.91 s** in the JVT stage. Systematic output writing took **1.99 s**, and JVT output writing **1.08 s**, inclusive. These are individual profiled costs rather than additive non-overlapping phase totals. The remaining performance work should investigate correction-table lookup/evaluation and allocation overhead; the benchmark also has substantial validation I/O that a production analysis may not need.', '',
'## RNG changes and agreement with A', '',
'Step 10 removes a redundant initialization: constructing `TRandom3` with its final seed replaces default construction followed by `SetSeed`. Step 11 caches the first standard Gaussian for each original seed in a bounded direct-mapped, thread-local table. Different resolution states reuse that draw with their own width. If the scaled first draw is negative, the original seeded ROOT rejection loop runs unchanged. Caching an accepted smeared value would be incorrect because rejection depends on the width.', '',
'The RNG algorithm, seed mapping, sign rules and systematic correlations are unchanged. This removes repeated RNG work without replacing ROOT with a different random stream. The table holds 1,048,576 entries, approximately 16 MiB per thread that uses smearing; one thread is the default. Full seed tags make collisions safe, with recomputation on a miss. No global cache is serialized, and no new user configuration is required.', '',
'Both RNG steps passed 96,060 exact reference comparisons covering zero/fallback seeds, physical and nonphysical phi values, forced collisions, active/inactive signs, large widths with rejection, and reversed traversal order. The existing six backend tests passed. Every timing repetition passed the existing full-sample nominal, systematic and JVT comparisons against A, without changing tolerances or seeds.', '',
'A direct comparison against the uncached single-thread checkpoint also found every saved physics-output buffer bitwise identical across all 117 kinematic states and 119 JVT/weight states, including event and jet identities. The intentionally changed backend fingerprint is excluded from this physics comparison. See `optimization-results/checks/full-smearing-parity.json`.', '',
f"The shared B overlay was rebuilt after each RNG change. The final full B integration passed against A on all 20,000 events using unchanged `config/reference.yaml` (SHA-256 `{b['config_sha256']}`). A single, CPU-affinity-restricted B integration run took **{b['wall_seconds']:.2f} s**; this is validation evidence, not a repeated B speedup measurement. Loaded-library mappings are retained with the report.", '',
'C-nice retains its existing nominal facade call, and the full workflow still composes that facade with the C systematic/JVT runners. The shared physics implementation is used by B and C; no Path D code is involved.', '',
'Reports: `optimization-results/09-profile/` and `optimization-results/11-profile/` contain before/after Python and native profiles. `optimization-results/11-B-integration/` contains the B check. Source profiling harnesses and raw generated artifacts remain in WSL `optimization-campaign/`. Timing summaries record the commit before the uncommitted candidate was measured; the table records the committed candidate that includes its measurements.', '',
'Reproduce a new timing checkpoint using `bash optimization-campaign/single-thread-campaign.sh NEW-LABEL` in the authoritative WSL checkout. Existing labels are protected from overwrite. Optional multi-thread support remains available explicitly, but all current reported timings use one CPU. All changes and reports are committed locally; nothing is pushed.']
report='\n'.join(lines)+'\n'
(windows/'single-thread-profiling.md').write_text(report)
notice=f"**Current single-threaded result: {final['mean']:.2f} ± {final['sd']:.2f} s.** The earlier 38.72 s result used four native workers. See [single-threaded timings and profiling](single-thread-profiling.md) for the current default, isolated RNG changes, and validation."
for name in ('c-nice-optimization.md','four-path-timing-comparison.md'):
    p=windows/name;s=p.read_text();first,body=s.split('\n',1)
    p.write_text(first+'\n\n'+notice+'\n'+body)
p=windows/'c-nice-optimization.md';s=p.read_text()
s=s.replace('## Outcome and acceptance evidence','## Historical four-thread outcome and acceptance evidence')
s=s.replace('Subsequent Python-only steps leave this shared C++ implementation unchanged.','Steps 04–08 leave this shared C++ implementation unchanged. The subsequent RNG changes and final B revalidation are documented in the single-thread follow-up.')
s=s.replace('| Step | Full C-nice wall seconds', 'Rows 05–08 use four native workers. Rows 09–11 use one native worker and one-CPU affinity. The change in execution policy is intentional.\n\n| Step | Full C-nice wall seconds')
p.write_text(s)
p=windows/'correctionlib-interface-handoff.md'
p.write_text(p.read_text()+'\n\n## Single-threaded follow-up and RNG profiling\n\n'+report.split('\n',1)[1])
p=windows/'c-nice/README.md';s=p.read_text()
s=s.replace('(default four, limited by CPU affinity; set to one for sequential evaluation)','(default one; set a larger value only for an explicit parallel experiment)')
s += f"\nCurrent full-systematics single-core timing is **{final['mean']:.2f} ± {final['sd']:.2f} s**. See [the RNG and profiling follow-up](../single-thread-profiling.md). The shared smearing implementation retains ROOT's exact random stream while avoiding redundant seeding and reusing first Gaussian draws with width-dependent rejection preserved.\n"
p.write_text(s)
for label in ('09-profile','11-profile','11-B-integration'):
    src=root/'optimization-campaign'/label;dst=windows/'optimization-results'/label;dst.mkdir(exist_ok=True)
    for p in src.iterdir():
        if p.is_file() and p.suffix in ('.txt','.json','.log'):
            shutil.copy2(p,dst/p.name)
checks=windows/'optimization-results/checks'
shutil.copy2(root/'optimization-campaign/checks/full-smearing-parity.json',checks/'full-smearing-parity.json')
for p in (root/'optimization-campaign/checks').glob('*rng*'):
    if p.suffix=='.log':shutil.copy2(p,checks/p.name)
for p in (root/'optimization-campaign/checks').glob('*gaussian*'):
    if p.suffix=='.log':shutil.copy2(p,checks/p.name)
for name in ('single-thread-profiling.md','c-nice-optimization.md','four-path-timing-comparison.md','correctionlib-interface-handoff.md','c-nice/README.md'):
    (root/name).write_text((windows/name).read_text().replace('](columnar-source/', ']('))
print(notice)
