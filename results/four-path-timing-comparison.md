# Four-path implementation and timing comparison

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
