# Validation report

PASS: the pinned demo meets the original numerical tolerances on all 20,000 events (196,606 jets).

| Comparison | Coverage | Result |
|---|---|---|
| A vs B | 1,190 branches; unchanged analysis configuration | PASS |
| A vs C | 117 nominal/JES/JER states and 119 NNJVT/weight states | PASS |
| B vs C | Same complete state coverage | PASS |
| Nominal calibration stages | Pile-up, eta/JES, GSC: all coordinates | Bitwise identical |
| Shuffled input, chunk sizes and workers | All 119 states, full input | Exactly invariant |
| PHYS A vs B | 100 events, 1,190 branches | PASS |
| Boundary tests | 110 eta/JES and 672 JVT cases | PASS |
| Unit tests | 6 tests | PASS |
| Athena 25.0.76 | Build and initialize five tools, zero events | PASS |

## Reproduction and evidence

Run scripts/run-demo.sh inside atlas_al9. See ../README.md for individual commands.
Full per-systematic differences and mismatch counts are in validation-summary.json and the linked reports.
The input SHA256 is 017e408002b0153df1de695d8c2ace325e0731b658cbd5ca695a349eb0a059c0.
Configuration SHA256: 4d84aa237b6a1796c05e9a38fa341e89993f1b8c582bc99da7b1624b4ed46ca8.

The CorrectionLib v2.7.0 base requires the included full-precision JSON parsing patch. No tolerance was relaxed.

## Limits

- Validation applies to this pinned configuration and sample; unsupported bundles fail explicitly.
- Athena validation is build and initialization only; AnalysisBase is the numerical authority.
- Timing is a component benchmark with possibly warm caches; feature reconstruction is not timed.
- Compiler manifests describe conversion-stage status; this report records subsequent end-to-end validation.
- The bundle-only smoke restricts DATAPATH; no system-call file-open audit was performed.

## Reports

- [nominal-comparison](nominal-comparison.json)
- [atlas-comparison](atlas-comparison.json)
- [systematics-comparison](systematics-comparison.json)
- [jvt-comparison](jvt-comparison.json)
- [bc-kinematics-comparison](bc-kinematics-comparison.json)
- [bc-jvt-comparison](bc-jvt-comparison.json)
- [determinism](determinism.json)
- [systematic-determinism](systematic-determinism.json)
- [phys-comparison](phys-comparison.json)
- [jes-boundaries](jes-boundaries.json)
- [jvt-boundaries](jvt-boundaries.json)

## Repeated timing exercise

All runs: 20,000 events, 196,606 jets, atlas_al9 WSL2; AnalysisBase 25.2.100 toolchain. Seconds of wall-clock time.

| Path | Run 1 (excluded) | Run 2 | Run 3 | Run 4 | Mean (2–4) | Sample SD (2–4) |
|---|---:|---:|---:|---:|---:|---:|
| A — untouched AnalysisBase | 242.03 | 237.90 | 236.16 | 237.72 | 237.26 | 0.96 |
| B — CorrectionLib ATLAS tools | 294.24 | 291.07 | 289.55 | 291.48 | 290.70 | 1.01 |
| C — columnar validation workflow | 168.91 | 167.85 | 168.57 | 166.68 | 167.70 | 0.95 |

Wall time after environment setup; imports, initialization, input preparation, computation and output included. C also includes reference comparisons and recalculates kinematics in its JVT stage. A/B comparisons are outside timed jobs. These measure existing workflows, not isolated evaluator speed.

No cache flushing; first run discarded per path; CVMFS/OS caches may already be warm.

Each run has its own outputs and logs under results/repeated-timing. All runs are checked against untouched AnalysisBase. See repeated-timing/summary.json for commands, individual phase times, and unrounded measurements.

Athena was not included: its previous test only initialized tools with zero events.

Implementation history: [code-reviewed build log](../docs/implementation-log.md).

## Four-path timing comparison


20,000 events / 196,606 jets per run. All times are wall seconds. Each path was run four times; its first run is excluded from averages and sample standard deviations.

| Metric | A: reference | B: ATLAS + CorrectionLib | C: columnar | D: declarative columnar |
|---|---|---|---|---|
| Implementation | Original ATLAS tools | Handwritten shared C++ physics + CorrectionLib payloads | Python input preparation + same shared C++ backend as B | Payload-defined physics + optional GraphSet extension and generic primitives |
| Run 1 (excluded) | 242.03 s | 294.24 s | 168.91 s | 255.91 s |
| Run 2 | 237.90 s | 291.07 s | 167.85 s | 305.23 s |
| Run 3 | 236.16 s | 289.55 s | 168.57 s | 283.89 s |
| Run 4 | 237.72 s | 291.48 s | 166.68 s | 260.50 s |
| **Average, runs 2–4** | **237.26 s** | **290.70 s** | **167.70 s** | **283.21 s** |
| Sample SD, runs 2–4 | 0.96 s | 1.01 s | 0.95 s | 22.37 s |
| Nominal stage average | Not separately measured | Not separately measured | 19.48 s | 23.90 s |
| JES/JER stage average | Not separately measured | Not separately measured | 53.94 s | 67.36 s |
| NNJVT/weights stage average | Not separately measured | Not separately measured | 94.28 s | 191.95 s |
| Timed reference comparison | Excluded | Excluded | Included | Included |
| Retained-run validation | PASS | PASS | PASS | PASS |

Sequential full diagnostic workflow after environment setup; includes imports, initialization, input/feature preparation, repeated kinematic evaluation, writing and reference comparisons. No cache flush; first run excluded. A/B/C timings are prior measurements, not interleaved with these D runs.

A/B process all stages within one ATLAS job, so separate phase costs are unavailable. C/D phase measurements include preparation, output and validation; the JVT phase repeats systematic evaluation. D timings use the diagnostic stage runners, not a new fused execution of the complete pipeline graphs. These are implemented-workflow costs, not isolated CorrectionLib evaluator benchmarks. Stage means can differ slightly from the total due to orchestration overhead.

D is interpreted graph execution with a versioned optional extension, not stock schema-v2-only functionality or JIT compilation. ROOT coordinate/TRandom3 operations and lwtnn inference remain explicit reusable primitives.

D raw measurements: path-d/results/repeated-timing/summary.json. A/B/C raw measurements: results/repeated-timing/summary.json. All isolated D run logs and outputs are retained beside its summary.
