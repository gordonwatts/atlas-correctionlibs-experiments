# C-nice full-systematics optimization campaign

**Current single-threaded result: 49.63 ± 0.39 s.** The earlier 38.72 s result used four native workers. See [single-threaded timings and profiling](single-thread-profiling.md) for the current default, isolated RNG changes, and validation.

20,000 events / 196,606 jets; all 117 kinematic and 119 JVT/weight states. Three sequential runs per step; first discarded, mean and sample SD of the remaining two. Setup and compilation excluded. Existing imports, input preparation, numerical evaluation, ROOT outputs and A-reference comparisons remain included. No random-seed changes. Timing differences include run-to-run machine variation.

Rows 05–08 use four native workers. Rows 09–11 use one native worker and one-CPU affinity. The change in execution policy is intentional.

| Step | Full C-nice wall seconds | Change and code reference | WSL implementation commit |
|---|---:|---|---|
| 00-baseline | 155.86 ± 0.51 | Fresh array-preparation baseline; unchanged evaluator and diagnostic outputs; [source](c-nice/atlas_jet_corrections.py) | `5997af7d848dc290e5c4f46d74c7ef79ba943416` |
| 01-array-bindings | 112.06 ± 0.42 | Native dense-array batch bindings eliminate Python scalar/list transfer; shared evaluators unchanged; [source](columnar-source/src/nominal.cxx) | `c6764e198e0259a2d81732d685dc115ca2665873` |
| 02-event-products | 108.25 ± 0.77 | Replace per-event Python weight products with float64 Awkward reductions; [source](columnar-source/scripts/run-columnar-jvt.py) | `0992cd9608f2b492e56e65d5f9445abb14b32972` |
| 03-compiled-metadata | 100.79 ± 0.33 | Parse uncertainty component metadata and reference constants once at construction; [source](columnar-source/src/systematics.hxx) | `d7667fb696d3fd2d382aab9f7d280aa53e8260ca` |
| 04-reuse-kinematics | 83.21 ± 0.88 | Reuse validated kinematics from the systematic stage in the JVT stage; [source](columnar-source/scripts/run-columnar-jvt.py) | `efecd4aeab0cda89e91b2e5a1afd94fd698c7959` |
| 05-parallel-batches | 61.05 ± 0.09 | Evaluate independent native batch slices on four threads; preserve scalar physics and ordering; [source](columnar-source/python/atlas_correctionlib/parallel.py) | `9d2af94fe7ead89669f018a87d36595c3cab8616` |
| 06-array-jvt-inputs | 54.41 ± 0.68 | Replace Python event loops for primary-vertex inputs and truth matching with Awkward arrays; [source](columnar-source/scripts/run-columnar-jvt.py) | `153f82cf579085f9acc148e52d9f2ab74b13f5c7` |
| 07-lz4-output | 43.26 ± 0.10 | Use LZ4 for the same ROOT output columns instead of default ZLIB; [source](columnar-source/scripts/run-columnar-systematics.py) | `2f2c39a78209c261a5acc3a6f2b9f4b5d5f3dd8d` |
| 08-lazy-metadata | 38.72 ± 0.29 | Load optional Python table/range inspection data only on access; native evaluator payloads unchanged; [source](columnar-source/python/atlas_correctionlib/jvt.py) | `492f657b4ecea85630d9df1f5ee733aded10870e` |
| 09-single-thread | 56.27 ± 0.03 | Default to one native worker; benchmark on one CPU with numerical library threads capped at one; [source](columnar-source/python/atlas_correctionlib/parallel.py) | `b79d363fe0a2d350a963802c912a3a543a4e18d9` |
| 10-rng-construction | 54.08 ± 0.42 | Construct TRandom3 with its final seed, eliminating redundant default seeding; [source](columnar-source/src/backend.hxx) | `b9b6bd2990b621240099fe4e9fcbcce41219a3f8` |
| 11-cached-gaussian | 49.63 ± 0.39 | Cache the first standard Gaussian by original seed; preserve rejection and ROOT stream exactly; [source](columnar-source/src/backend.hxx) | `1813b4e8136d8539b74fdca469742ef277a86530` |

Every row passed all full-sample nominal, systematic and JVT reference comparisons. Per-run reports and raw timings are retained in `optimization-results/`. The facade remains nominal-only; full systematic timings refer to its nominal stage followed by the established C systematic/JVT runners. B shares the physics implementation. B configuration is unchanged; any shared-backend change also requires its own B integration validation before acceptance.

B integration after step 03: all 20,000 events passed comparison with A using unchanged `config/reference.yaml` (SHA-256 `4d84aa237b6a1796c05e9a38fa341e89993f1b8c582bc99da7b1624b4ed46ca8`). One run took 288.82 s; this is an integration check, not a repeated speedup measurement. Saved worker memory mappings confirm the rebuilt overlay libraries were loaded. Steps 04–08 leave this shared C++ implementation unchanged. The subsequent RNG changes and final B revalidation are documented in the single-thread follow-up.

## Historical four-thread outcome and acceptance evidence

The full-systematics workflow fell from **155.86 ± 0.51 s** to **38.72 ± 0.29 s**, a **4.03× speedup** and **75.2% reduction** on this frozen sample and host. These are sequential checkpoint measurements with two retained observations each, not an interleaved statistical study or a claim about every input sample. Nominal recalibration, all systematic states, output writing and reference comparisons remain enabled. Neither the seeds nor the acceptance tolerances changed.

The first two major savings remove Python scalar transfer and repeated work. B and C still use the same native physics equations and payloads. The only physics-backend implementation optimization compiles JSON metadata into typed fields once; B was rebuilt and its full-sample A comparison passed with its original configuration. Later changes affect the Python workflow and output encoding. B's single post-change integration run was 288.82 s; it is not a repeated B performance measurement.

Four native worker threads trade additional CPU and temporary batch buffers for elapsed time. Set `ATLAS_CORRECTIONLIB_THREADS=1` before Python startup to disable concurrency. Four-thread versus sequential evaluation was bitwise identical for 117 kinematic and 119 JVT/weight states on 16,384 jets. Native dense-array versus legacy list bindings were also checked bitwise. Full-sample array-based JVT input preparation matched the prior features bitwise. Empty/strided/malformed batch inputs and the existing six backend tests were checked. Cache rejection checks cover changed fingerprints, states, events, jet ordering and correction statuses. Supporting checks are saved in `optimization-results/checks/`.

LZ4 trades file size for speed: for retained run 2, the kinematics file grew from approximately 442 MiB to 595 MiB and the JVT file from 115 MiB to 194 MiB. All columns remain present and validated. Reusing those kinematics avoids a second numerical calculation, while retaining the file read and provenance checks in the timer. Array truth matching forms event-local jet/truth Cartesian pairs; memory demand will depend on multiplicity. Peak resident memory was not measured in this campaign.

The public C-nice call and B configuration remain the same. The facade still exposes nominal calibration; this full-systematics benchmark composes it with the existing C systematic/JVT runners. It does not add a systematics facade API or switch to Path D. Python event/jet loops in the JVT preparation and event-weight reduction have been replaced with arrays; native scalar evaluation and Python loops over chunks, fields and systematic names remain.

Each implementation row identifies the authoritative WSL commit, including its measured evidence. Windows commits preserve inspection copies and this report. Generated ROOT files remain in the corresponding WSL campaign directories; lightweight timings, comparisons and source snapshots are committed. To reproduce a new checkpoint, source `setup.sh`, export `PYTHONPATH="$PWD/c-nice:$PWD/install/python:$PYTHONPATH"`, and run `python optimization-campaign/optimization-campaign.py NEW-LABEL` from `/home/gwatts/atlas/correctionlib_tests`. Existing labels fail instead of overwriting earlier results. Setup/build time is outside these measurements. No remote push was made.


The subsequent exact-result single-core campaign selected **26.44 ± 0.42 s** (1.88× versus 49.63 s; 1.78× versus its rebuilt control). See [the per-change table and acceptance report](x2-optimization.md). All A comparisons and outputs remain enabled, final C arrays match the prior checkpoint byte for byte, and B passed with unchanged configuration. The 5,000-event chunk experiment was reverted; 1,000-event chunks remain selected.
