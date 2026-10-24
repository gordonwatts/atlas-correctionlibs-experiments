# Single-threaded C-nice and RNG profiling

Current full-systematics C-nice runtime: **49.63 ± 0.39 s**, on one CPU. The four-thread result from the previous campaign is historical. The default native worker count is now one. This follow-up reduces the single-core baseline of 56.27 s by 11.8%.

Each checkpoint processes the same 20,000 events / 196,606 jets and all 117 kinematic and 119 JVT/weight states. Three sequential runs per checkpoint, first discarded; mean and sample SD use the remaining two. Numerical-library thread limits are one and CPU affinity is restricted to one logical CPU, including child processes. This also prevents incidental reader/decompression workers from contributing parallel CPU throughput. Initialization, nominal recalibration, all outputs and A-reference validation remain inside the timer; setup and compilation remain outside. The earlier four-thread campaign did not use this affinity restriction.

| Change, cumulative | Full workflow seconds | Shared source | WSL implementation commit |
|---|---:|---|---|
| Default to one native worker; benchmark on one CPU with numerical library threads capped at one | 56.27 ± 0.03 | [source](python/atlas_correctionlib/parallel.py) | `b79d363fe0a2d350a963802c912a3a543a4e18d9` |
| Construct TRandom3 with its final seed, eliminating redundant default seeding | 54.08 ± 0.42 | [source](src/backend.hxx) | `b9b6bd2990b621240099fe4e9fcbcce41219a3f8` |
| Cache the first standard Gaussian by original seed; preserve rejection and ROOT stream exactly | 49.63 ± 0.39 | [source](src/backend.hxx) | `1813b4e8136d8539b74fdca469742ef277a86530` |

## What the profiles show

The first optimization campaign used stage timing and targeted comparisons, rather than a function-level profile. This follow-up adds cProfile across the complete three-stage workflow and Callgrind for the shared native systematic evaluator. Native profiling uses 2,048 evenly spaced jets from the frozen sample, all 117 states, and the same compiler toolchain at -O2. Payload initialization is outside the native collection region. Python profiling retains full sample initialization, I/O and validation. Profiled elapsed times include instrumentation overhead and must not be substituted for the uninstrumented timing table.

Before the RNG changes, the native profile attributed **15.96% of instructions to `TRandom3::SetSeed` itself** and **6.97% to `TRandom3::Rndm` itself**. RNG work is therefore material. The table lookup/evaluation path accounted for **65.73% inclusive** of instructions, including work in its callees. Inclusive percentages overlap; these figures are instruction counts, not wall-time percentages.

After the Gaussian-cache change, the same native profile attributes **0.76%** to seeding and **0.67%** to random-number generation itself. The original RNG stream is retained, while repeated RNG work is largely removed.

Profiled systematic-batch time falls from **22.08 s to 13.82 s**. The table lookup/evaluation path now accounts for **85.42% inclusive** of native instructions; its absolute instruction count is essentially unchanged. It is the next substantial native optimization target. The uninstrumented final phase means are approximately 6.15 s nominal, 24.37 s systematic, and 19.11 s JVT, including their I/O and validation.

The baseline Python profile attributed about **22.08 s** to the 117 native systematic batch calls. Reading A-reference branches took **8.55 s** in the systematic stage and **6.91 s** in the JVT stage. Systematic output writing took **1.99 s**, and JVT output writing **1.08 s**, inclusive. These are individual profiled costs rather than additive non-overlapping phase totals. The remaining performance work should investigate correction-table lookup/evaluation and allocation overhead; the benchmark also has substantial validation I/O that a production analysis may not need.

## RNG changes and agreement with A

Step 10 removes a redundant initialization: constructing `TRandom3` with its final seed replaces default construction followed by `SetSeed`. Step 11 caches the first standard Gaussian for each original seed in a bounded direct-mapped, thread-local table. Different resolution states reuse that draw with their own width. If the scaled first draw is negative, the original seeded ROOT rejection loop runs unchanged. Caching an accepted smeared value would be incorrect because rejection depends on the width.

The RNG algorithm, seed mapping, sign rules and systematic correlations are unchanged. This removes repeated RNG work without replacing ROOT with a different random stream. The table holds 1,048,576 entries, approximately 16 MiB per thread that uses smearing; one thread is the default. Full seed tags make collisions safe, with recomputation on a miss. No global cache is serialized, and no new user configuration is required.

Both RNG steps passed 96,060 exact reference comparisons covering zero/fallback seeds, physical and nonphysical phi values, forced collisions, active/inactive signs, large widths with rejection, and reversed traversal order. The existing six backend tests passed. Every timing repetition passed the existing full-sample nominal, systematic and JVT comparisons against A, without changing tolerances or seeds.

A direct comparison against the uncached single-thread checkpoint also found every saved physics-output buffer bitwise identical across all 117 kinematic states and 119 JVT/weight states, including event and jet identities. The intentionally changed backend fingerprint is excluded from this physics comparison. See `optimization-campaign/checks/full-smearing-parity.json`.

The shared B overlay was rebuilt after each RNG change. The final full B integration passed against A on all 20,000 events using unchanged `config/reference.yaml` (SHA-256 `4d84aa237b6a1796c05e9a38fa341e89993f1b8c582bc99da7b1624b4ed46ca8`). A single, CPU-affinity-restricted B integration run took **281.61 s**; this is validation evidence, not a repeated B speedup measurement. Loaded-library mappings are retained with the report.

C-nice retains its existing nominal facade call, and the full workflow still composes that facade with the C systematic/JVT runners. The shared physics implementation is used by B and C; no Path D code is involved.

Reports: `optimization-campaign/09-profile/` and `optimization-campaign/11-profile/` contain before/after Python and native profiles. `optimization-campaign/11-B-integration/` contains the B check. Source profiling harnesses and raw generated artifacts remain in WSL `optimization-campaign/`. Timing summaries record the commit before the uncommitted candidate was measured; the table records the committed candidate that includes its measurements.

Reproduce a new timing checkpoint using `bash optimization-campaign/single-thread-campaign.sh NEW-LABEL` in the authoritative WSL checkout. Existing labels are protected from overwrite. Optional multi-thread support remains available explicitly, but all current reported timings use one CPU. All changes and reports are committed locally; nothing is pushed.


The subsequent exact-result single-core campaign selected **26.44 ± 0.42 s** (1.88× versus 49.63 s; 1.78× versus its rebuilt control). See [the per-change table and acceptance report](x2-optimization.md). All A comparisons and outputs remain enabled, final C arrays match the prior checkpoint byte for byte, and B passed with unchanged configuration. The 5,000-event chunk experiment was reverted; 1,000-event chunks remain selected.
