# Complete C-nice optimization timing history

**Selected full-systematics result: 25.61 ± 0.31 s on one CPU.** Both NN changes are implemented in the shared B/C inference backend. This is 1.94× faster than the 49.63 s checkpoint and 0.83 s below the pre-NN 26.44 s checkpoint. These are short sequential measurements, not a controlled estimate of each change’s isolated gain.

Checkpoint 26 is selected. Checkpoint 21’s larger input chunks were reverted. Checkpoints 18 and 25 have no individually established performance gain. See [the NN implementation and acceptance report](nn-optimization.md).

The table includes every optimization checkpoint from the original 155.86 s baseline. Steps 05–08 used four native workers; steps 09 onward use one worker pinned to one CPU. Earlier steps 00–04 used one native worker without the later affinity policy. Do not interpret the thread-policy transitions as isolated implementation gains. Historical A/B/C/D, nominal-only and no-calibration comparisons remain in [the four-path comparison](four-path-timing-comparison.md). Profiling runs 22–24 are diagnostic studies, not additional accepted benchmark checkpoints.

Three runs per checkpoint; discard the first and report mean/sample SD of the remaining two. Runs were sequential, not interleaved, with no forced OS-cache flush. These are short checkpoint measurements on one frozen sample, not precision estimates of production throughput. One-CPU affinity, one native worker and one BLAS/OpenMP thread were enforced. Setup and builds are outside the timer; Python startup, nominal recalibration, all systematic/JVT processing, input/output and every A reference comparison are inside. Nominal-only plots and no-calibration timing were not rerun.

| Step | Seconds | Change and source | Authoritative WSL commit |
|---|---:|---|---|
| 00-baseline | 155.86 ± 0.51 | Fresh array-preparation baseline; unchanged evaluator and diagnostic outputs; [source](c-nice/atlas_jet_corrections.py) | `5997af7d848dc290e5c4f46d74c7ef79ba943416` |
| 01-array-bindings | 112.06 ± 0.42 | Native dense-array batch bindings eliminate Python scalar/list transfer; shared evaluators unchanged; [source](src/nominal.cxx) | `c6764e198e0259a2d81732d685dc115ca2665873` |
| 02-event-products | 108.25 ± 0.77 | Replace per-event Python weight products with float64 Awkward reductions; [source](scripts/run-columnar-jvt.py) | `0992cd9608f2b492e56e65d5f9445abb14b32972` |
| 03-compiled-metadata | 100.79 ± 0.33 | Parse uncertainty component metadata and reference constants once at construction; [source](src/systematics.hxx) | `d7667fb696d3fd2d382aab9f7d280aa53e8260ca` |
| 04-reuse-kinematics | 83.21 ± 0.88 | Reuse validated kinematics from the systematic stage in the JVT stage; [source](scripts/run-columnar-jvt.py) | `efecd4aeab0cda89e91b2e5a1afd94fd698c7959` |
| 05-parallel-batches | 61.05 ± 0.09 | Evaluate independent native batch slices on four threads; preserve scalar physics and ordering; [source](python/atlas_correctionlib/parallel.py) **Historical four-worker run.** | `9d2af94fe7ead89669f018a87d36595c3cab8616` |
| 06-array-jvt-inputs | 54.41 ± 0.68 | Replace Python event loops for primary-vertex inputs and truth matching with Awkward arrays; [source](scripts/run-columnar-jvt.py) **Historical four-worker run.** | `153f82cf579085f9acc148e52d9f2ab74b13f5c7` |
| 07-lz4-output | 43.26 ± 0.10 | Use LZ4 for the same ROOT output columns instead of default ZLIB; [source](scripts/run-columnar-systematics.py) **Historical four-worker run.** | `2f2c39a78209c261a5acc3a6f2b9f4b5d5f3dd8d` |
| 08-lazy-metadata | 38.72 ± 0.29 | Load optional Python table/range inspection data only on access; native evaluator payloads unchanged; [source](python/atlas_correctionlib/jvt.py) **Historical four-worker run.** | `492f657b4ecea85630d9df1f5ee733aded10870e` |
| 09-single-thread | 56.27 ± 0.03 | Default to one native worker; benchmark on one CPU with numerical library threads capped at one; [source](python/atlas_correctionlib/parallel.py) | `b79d363fe0a2d350a963802c912a3a543a4e18d9` |
| 10-rng-construction | 54.08 ± 0.42 | Construct TRandom3 with its final seed, eliminating redundant default seeding; [source](src/backend.hxx) | `b9b6bd2990b621240099fe4e9fcbcce41219a3f8` |
| 11-cached-gaussian | 49.63 ± 0.39 | Cache the first standard Gaussian by original seed; preserve rejection and ROOT stream exactly; [source](src/backend.hxx) | `1813b4e8136d8539b74fdca469742ef277a86530` |
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
| 25-nn-feature-cache | 27.01 ± 1.00 | Cache exact NN input tuples in a bounded model-scoped native cache shared by B and C. **Gain inconclusive.** [Source](src/nn_graph.hxx) | `e35bd9d9a171aee5eeecf68016f44aea058b0e1f` |
| 26-nn-reusable-buffers | 25.61 ± 0.31 | Reuse NN intermediate buffers with matching Eigen operations in the shared B/C backend [Source](src/nn_graph.hxx) | `2d8cad68da093634555819df49ea7190e4285023` |

## Earlier campaign through checkpoint 20

The earlier profile identified table evaluation as the dominant remaining native work. The first change removes a copy of the non-uniform bin-edge vector on every lookup. The original full-precision JSON patch was preserved separately before rebuilding. The 49.63-to-47.13-second control shift is not credited to the bin-copy fix. The library was built with the same Release settings before and after that fix; exact external-source commits and installed library SHA-256 hashes are retained in each checkpoint’s implementation metadata.

The shared backend now binds immutable correction objects during construction, avoiding string construction and map lookup for each jet. Grouped C evaluation computes the common uncertainty once for related up/down and MC/PseudoData states. B scalar evaluation and C grouped evaluation call the same arithmetic and smearing functions. Contiguous adapters reuse native input storage and write results directly into NumPy buffers, avoiding millions of small intermediate allocations. Native scalar loops remain; Python input preparation and event reductions remain array operations.

The diagnostic runner now executes nominal, systematic and JVT stages in one process, sharing loaded objects. Each original stage can still run independently. It still writes and rereads the saved kinematics and checks provenance, event identities, jet ordering and statuses. Reference reads are grouped into blocks of 16 branches; the same reference values and tolerances are used. The bounded JVT cache holds one exact sigma-zero result and input copies; changes to effective inputs or tool objects miss the cache, and returned arrays cannot mutate cached data.

The C-nice facade remains nominal-only and its call is unchanged. Full-systematics timing composes it with the C diagnostic stages; this does not add a new systematic facade or use Path D. B configuration is unchanged. As before, this diagnostic workflow explicitly reruns nominal calibration for its staged A comparison. Normal PHYSLITE analysis can use stored nominal jets; this is not a claim that nominal recalibration is required for normal analysis.

Keeping arrays and tools alive trades memory for startup/read savings. The JVT anchor is proportional to one batch and the reference reader retains one 16-branch block. Grouped native output holds at most four states at once; temporary per-jet vectors have been removed from the array adapters. Peak resident memory was not measured, and multiplicity-dependent truth matching remains a memory consideration. The selected input chunk size is still 1,000 events.

## Validation and remaining cost at checkpoint 20

Every repetition passed the full A nominal, systematic and JVT comparisons. Final raw-byte comparisons against checkpoint 11 passed for all prepared input arrays, nominal stages, 117 kinematic states and 119 JVT/weight states, including dtype, event/jet identities and statuses. Only the intentionally changed backend-provenance metadata is excluded. Evidence: [final parity](optimization-campaign/x2-checks/11-cached-gaussian-to-20-flat-adapters-parity.json).

The existing six backend tests passed after each native build. The grouped API was checked against scalar evaluation across all 117 states, including empty/strided/malformed inputs and invalid groups. Dense adapters were checked against legacy bindings, including nominal stages. JVT-cache checks covered changed inputs, mutation, sigma shifts, replacement tools and empty batches. The initial integrated-runner attempt failed because counts initialization was accidentally conditional on disk loading; this was fixed before acceptance, and its failed logs remain under `16-integrated-workflow`.

B integration at checkpoint 20 passed all 20,000 events against A with unchanged `config/reference.yaml` (SHA-256 `4d84aa237b6a1796c05e9a38fa341e89993f1b8c582bc99da7b1624b4ed46ca8`). The one-core B run took **271.04 s**. This is a single integration measurement, not a repeated B speedup estimate. Process mappings confirm the rebuilt overlay and expected CorrectionLib library were loaded. See [B summary](optimization-campaign/x2-B-integration/summary.json) and [loaded libraries](optimization-campaign/x2-B-integration/loaded-libraries.txt).

The checkpoint-20 uninstrumented stage means are nominal **4.43 s**, systematics **9.86 s**, and JVT **11.74 s**, plus process startup/teardown. Each stage includes its own I/O and validation, so these are not pure kernel times. At checkpoint 20, a further 2× claim needed another 6.2% reduction from the selected result. Before accepting approximations, the next useful experiment is a fresh profile separating JVT computation from reading/writing/reference comparison, under matched production-like output requirements.

All implementation checkpoints, the reverted experiment, verification evidence and reports are preserved in local Git. The authoritative source/build is WSL `atlas_al9:/home/gwatts/atlas/correctionlib_tests`; Windows contains synchronized inspection sources and lightweight results. Raw generated ROOT files remain in their original WSL campaign folders. The external CorrectionLib change is also committed in its own repository and exported as patches. Nothing was pushed. The timer summaries record the pre-change HEAD; the table gives the candidate commit containing the implementation and its measurements.

A fresh profile of the selected implementation is available in [the final profile report](final-profile-report.md), with direct wall-time attribution for reference reads, JVT, tool loading and output.

The subsequent [native calculation study](calculation-profile-report.md) resolves NNJVT, efficiency and grouped-systematic costs, including internal allocations and exact repeated NN features.


## Earlier cross-path, plotting and no-calibration timings

These are historical measurements with different scopes; none was remeasured for the NN changes. A/B full-job reference comparisons are outside their timers, while C/C-nice/D diagnostic comparisons are inside. Nominal plotting reruns nominal calibration. Stored-jet rows bypass calibration; the A stored-jet plot uses an intermediate tree. Do not treat these rows as matched reader or facade-overhead benchmarks.

| Path / workflow | Mean wall seconds | Sample SD | Measurement |
|---|---:|---:|---|
| A: original ATLAS, full workflow | 237.26 | 0.96 | Previous baseline |
| B: shared backend in ATLAS, full workflow | 290.70 | 1.01 | Previous baseline; backend unchanged |
| C-nice: array facade nominal + C systematics/JVT | 161.89 | 3.55 | Remeasured after array update |
| C-nice: array nominal file-to-Hist plot only | 8.05 | 0.06 | Remeasured; smaller scope |
| Stored PHYSLITE pT/eta to Hist; no calibration | 1.08 | 0.05 | Two input branches |
| Stored PHYSLITE to Hist; also read C-nice inputs | 2.22 | 0.02 | I/O control; no calibration |
| A: stored jets, no calibration/systematics, write tree | 8.75 | 0.37 | New CPRun measurement |
| A: above job plus Hist plot of output tree | 9.58 | 0.37 | Includes intermediate tree I/O |
| Initial C-nice before array update: full workflow | 188.53 | 2.31 | Earlier campaign |
| Initial C-nice before array update: nominal plot | 28.45 | 0.13 | Earlier campaign |
| Original C: full workflow | 167.70 | 0.95 | Historical four-path campaign |
| D: full workflow | 283.21 | 22.37 | Historical four-path campaign |

## B integration timings after backend changes

These are individual validation runs, not repeated benchmark means.

| After checkpoint | B wall seconds | Validation |
|---|---:|---|
| 03 | 288.82 | Full A comparison passed |
| 11 | 281.61 | Full A comparison passed |
| 20 | 271.04 | Full A comparison passed |
| 26 | 268.07 | Full A comparison passed; optimized NN tagger loaded |
