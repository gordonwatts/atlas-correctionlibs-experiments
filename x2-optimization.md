# Exact-result single-core optimization toward 2×

The selected full-systematics workflow takes **26.44 ± 0.42 s** on one CPU for 20,000 events (196,606 jets), down from **49.63 ± 0.39 s**: **1.88× faster**, or **46.7% less time**. It remains 1.63 s above the 24.81 s target. Against the freshly rebuilt unchanged control (47.13 s), the speedup is **1.78×**. No numerical approximation, seed change, tolerance change, systematic reduction, or dropped output/check was used.

The selected code is checkpoint 20. The larger-chunk experiment at checkpoint 21 showed no useful improvement and was reverted; its results remain preserved. The exact JVT cache at checkpoint 18 has no separately established speedup: its apparent gain is smaller than the observed timing variation.

Three runs per checkpoint; discard the first and report mean/sample SD of the remaining two. Runs were sequential, not interleaved, with no forced OS-cache flush. These are short checkpoint measurements on one frozen sample, not precision estimates of production throughput. One-CPU affinity, one native worker and one BLAS/OpenMP thread were enforced. Setup and builds are outside the timer; Python startup, nominal recalibration, all systematic/JVT processing, input/output and every A reference comparison are inside. Nominal-only plots and no-calibration timing were not rerun.

| Step | Seconds | Change and source | Authoritative WSL commit |
|---|---:|---|---|
| 12-rebuild-control | 47.13 ± 0.03 | Rebuild unchanged full-precision CorrectionLib with a reproducible Release configuration [Source](vendor-patches/existing-full-precision-json.patch) | `dc6d525e21d1fb49b72d7f74a1cfc0169e987af9` |
| 13-bin-edge-reference | 42.97 ± 0.00 | Reference non-uniform bin edges instead of copying their vector on every lookup [Source](vendor-patches/bin-edge-reference.patch) | `eb8b0a0c184b4c43a9c6a808d62e516e477903f9` |
| 14-bound-tables | 37.88 ± 0.55 | Resolve shared systematic correction objects once at construction [Source](src/systematics.hxx) | `abc0c38557e0073706bda25c232ba50c1919af19` |
| 15-paired-systematics | 34.90 ± 0.04 | Reuse exact shared uncertainty evaluation between related systematic states [Source](src/systematics.hxx) | `2c016e4457f5abdeed7254a1e337483a1bddc3ea` |
| 16-integrated-workflow-v2 | 31.71 ± 0.14 | Share loaded arrays and payloads between stages while retaining all outputs and checks [Source](scripts/run-columnar-full.py) | `a12daa8f55a67ca74d98fa64fce2c7d70e9fa383` |
| 17-batched-reference | 30.45 ± 0.19 | Read validation reference branches in bounded groups of 16 [Source](python/atlas_correctionlib/reference.py) | `21ca2601b2c3b39ae5c515a52cfb028494b6e8dc` |
| 18-exact-jvt-reuse | 30.35 ± 0.99 | Reuse JVT results only for exactly identical effective inputs **Gain inconclusive.** [Source](python/atlas_correctionlib/jvt.py) | `d0624b98d559e9945db30e06f680961205841e77` |
| 19-flat-group-buffers | 28.48 ± 0.12 | Write grouped native results directly into contiguous NumPy storage [Source](src/nominal.cxx) | `03888d2720da6d1d5f2ae30710cd02ae2a8237a8` |
| 20-flat-adapters | 26.44 ± 0.42 | Reuse one native input row and write scalar outputs directly into NumPy buffers [Source](src/nominal.cxx) | `38935839d523edfad166fcb740fdcb79deb8ed88` |
| 21-larger-read-chunks | 26.64 ± 0.41 | Read nominal inputs in 5000-event chunks instead of 1000; same full sample and outputs **Reverted: no useful gain.** [Source](scripts/run-columnar-full.py) | `eab8381bcf2cd209ea60e91230f8dfcca504af9f` |

## What changed

The earlier profile identified table evaluation as the dominant remaining native work. The first change removes a copy of the non-uniform bin-edge vector on every lookup. The original full-precision JSON patch was preserved separately before rebuilding. The 49.63-to-47.13-second control shift is not credited to the bin-copy fix. The library was built with the same Release settings before and after that fix; exact external-source commits and installed library SHA-256 hashes are retained in each checkpoint’s implementation metadata.

The shared backend now binds immutable correction objects during construction, avoiding string construction and map lookup for each jet. Grouped C evaluation computes the common uncertainty once for related up/down and MC/PseudoData states. B scalar evaluation and C grouped evaluation call the same arithmetic and smearing functions. Contiguous adapters reuse native input storage and write results directly into NumPy buffers, avoiding millions of small intermediate allocations. Native scalar loops remain; Python input preparation and event reductions remain array operations.

The diagnostic runner now executes nominal, systematic and JVT stages in one process, sharing loaded objects. Each original stage can still run independently. It still writes and rereads the saved kinematics and checks provenance, event identities, jet ordering and statuses. Reference reads are grouped into blocks of 16 branches; the same reference values and tolerances are used. The bounded JVT cache holds one exact sigma-zero result and input copies; changes to effective inputs or tool objects miss the cache, and returned arrays cannot mutate cached data.

The C-nice facade remains nominal-only and its call is unchanged. Full-systematics timing composes it with the C diagnostic stages; this does not add a new systematic facade or use Path D. B configuration is unchanged. As before, this diagnostic workflow explicitly reruns nominal calibration for its staged A comparison. Normal PHYSLITE analysis can use stored nominal jets; this is not a claim that nominal recalibration is required for normal analysis.

Keeping arrays and tools alive trades memory for startup/read savings. The JVT anchor is proportional to one batch and the reference reader retains one 16-branch block. Grouped native output holds at most four states at once; temporary per-jet vectors have been removed from the array adapters. Peak resident memory was not measured, and multiplicity-dependent truth matching remains a memory consideration. The selected input chunk size is still 1,000 events.

## Validation and remaining cost

Every repetition passed the full A nominal, systematic and JVT comparisons. Final raw-byte comparisons against checkpoint 11 passed for all prepared input arrays, nominal stages, 117 kinematic states and 119 JVT/weight states, including dtype, event/jet identities and statuses. Only the intentionally changed backend-provenance metadata is excluded. Evidence: [final parity](optimization-campaign/x2-checks/11-cached-gaussian-to-20-flat-adapters-parity.json).

The existing six backend tests passed after each native build. The grouped API was checked against scalar evaluation across all 117 states, including empty/strided/malformed inputs and invalid groups. Dense adapters were checked against legacy bindings, including nominal stages. JVT-cache checks covered changed inputs, mutation, sigma shifts, replacement tools and empty batches. The initial integrated-runner attempt failed because counts initialization was accidentally conditional on disk loading; this was fixed before acceptance, and its failed logs remain under `16-integrated-workflow`.

Final B integration passed all 20,000 events against A with unchanged `config/reference.yaml` (SHA-256 `4d84aa237b6a1796c05e9a38fa341e89993f1b8c582bc99da7b1624b4ed46ca8`). The one-core B run took **271.04 s**. This is a single integration measurement, not a repeated B speedup estimate. Process mappings confirm the rebuilt overlay and expected CorrectionLib library were loaded. See [B summary](optimization-campaign/x2-B-integration/summary.json) and [loaded libraries](optimization-campaign/x2-B-integration/loaded-libraries.txt).

The selected uninstrumented stage means are nominal **4.43 s**, systematics **9.86 s**, and JVT **11.74 s**, plus process startup/teardown. Each stage includes its own I/O and validation, so these are not pure kernel times. A further 2× claim needs another 6.2% reduction from the selected result. Before accepting approximations, the next useful experiment is a fresh profile separating JVT computation from reading/writing/reference comparison, under matched production-like output requirements.

All implementation checkpoints, the reverted experiment, verification evidence and reports are preserved in local Git. The authoritative source/build is WSL `atlas_al9:/home/gwatts/atlas/correctionlib_tests`; Windows contains synchronized inspection sources and lightweight results. Raw generated ROOT files remain in their original WSL campaign folders. The external CorrectionLib change is also committed in its own repository and exported as patches. Nothing was pushed. The timer summaries record the pre-change HEAD; the table gives the candidate commit containing the implementation and its measurements.

A fresh profile of the selected implementation is available in [the final profile report](final-profile-report.md), with direct wall-time attribution for reference reads, JVT, tool loading and output.
