# Original ATLAS versus CorrectionLib lookup timing

On this sample, the current CorrectionLib lookup path is slower than the original ATLAS/ROOT lookup. Reusing CorrectionLib argument vectors closes part of the gap; it is a benchmark-only experiment here. The full shared systematic replay differs by about two tenths of a second. No production backend or payload was changed.

## Matched lookup measurements

Median nanoseconds per lookup (JVT rows are per content/error pair), excluding the first of seven rounds. Every retained round rotates the order of the three implementations. All data and objects are loaded before timing; table names are resolved once.

| Lookup | Tables | ATLAS/ROOT ns | CorrectionLib ns | CorrectionLib with reused arguments ns | Current / ATLAS |
|---|---:|---:|---:|---:|---:|
| 1D-Full | 25 | 33.38 | 40.82 | 34.53 | 1.22× |
| 2D-Full | 17 | 104.89 | 130.10 | 119.10 | 1.24× |
| 2D-OnlyX | 16 | 57.22 | 61.80 | 51.23 | 1.08× |
| JvtFixedEffPt content + error | 1 | 24.78 | 54.20 | 36.64 | 2.19× |
| EffFixedEffPt content + error | 1 | 24.42 | 54.09 | 37.01 | 2.22× |

The interpolation benchmark covers all 58 distinct table/mode objects bound by the selected systematic evaluator. Each table is swept over all 196,606 nominal jets; this microbenchmark uses pT in GeV and absolute eta and weights each table equally. It is not the production frequency distribution. The shared-evaluator test below uses the original recipe-dependent signed/absolute eta, conditions, component combinations and call frequencies.

The original uncertainty implementation is the stock AnalysisBase 25.2.100 `jet::UncertaintyHistogram`, loaded from CVMFS, including its boundary handling, interpolation and cached projections. CorrectionLib uses the existing exported tables and the installed full-precision/bin-edge-reference-patched library. The reused-argument column still executes the same CorrectionLib graph; it only avoids creating the outer argument vector for every call. It is not a production optimization result.

For JVT, the benchmark reproduces the original `JvtEfficiencyToolBase.cxx` helper: find the two bins once, then retrieve content and error. CorrectionLib uses two named corrections, so it searches twice. Both receive the same 58,777 valid nominal-jet points, repeated 64 times per sweep, and return float content/error values. Input filtering happens before timing; ROOT retains its helper validity checks, while CorrectionLib is given prevalidated inputs. This is a lookup-pair comparison, not the complete efficiency weighting algorithm.

## Swap only the lookup in our shared systematic evaluator

A test-only copy of `systematics.hxx` changes `Table::operator()` to select the original ATLAS histogram or the existing CorrectionLib correction. Everything else uses the current shared backend, including its grouped API, arithmetic, RNG and exact Gaussian cache. Both paths are warmed before timing. The replay processes all 196,606 jets, 45 groups and 117 output states.

| Lookup used by shared backend | Mean ± sample SD | Median |
|---|---:|---:|
| Original ATLAS | 1.1146 ± 0.0117 s | 1.1098 s |
| CorrectionLib | 1.3074 ± 0.0736 s | 1.2946 s |

CorrectionLib adds **0.193 s** in the mean of this replay (**17.3%** relative to the ATLAS lookup version). Six retained alternating-order measurements follow one excluded warm-up round. This includes native input-row copying, result writes and shared physics; excludes Python, file reading, tool loading, validation, nominal calibration and JVT. Preallocated output is overwritten group by group. The additional runtime lookup-selection branch is present in both alternatives. This is not a measurement of Path A or B as a whole.

## Numerical checks and provenance

- 11,403,148 direct uncertainty lookups and 97,932 edge/nextafter checks passed at absolute tolerance 1e-10 plus relative tolerance 1e-9. The 2D interpolation has tiny rounding differences (largest sample absolute difference 6.66e-16); it is not claimed bit-identical.
- All 115,014,510 output values in the shared systematic comparison were exactly equal. Existing kinematic tolerance checks also passed. This is equality of the tested numeric values, not a binary-file comparison.
- Both JVT content/error comparisons were exactly equal after the original float conversion. Invalid-input and JVT boundary behavior are outside this timing study; the systematic boundary checks do not cover JVT.
- One CPU (CPU 0), compiled with `-O2 -g -DNDEBUG`, no fast-math. Loading, ROOT projection setup and validation are outside timers. Short cache-warm native microbenchmarks; ratios should not be extrapolated to startup or full-workflow timing.
- Source/payload/input hashes and loaded-library paths were checked. Production source hashes remained unchanged. The original 28-lookup-comparison directory retains the first compile attempt; it needed the standalone AnalysisBase compiler definition. The accepted results are in 28-lookup-comparison-v2.
- Nominal JES/GSC evaluation, NN inference and complete A/B/C workflows were not benchmarked here. No alternative production backend is selected by this study.

CorrectionLib therefore contributes payload portability and a common evaluator here, rather than an observed lookup speed advantage. The cost depends on representation: generic formula/transform traversal and argument handling add work; separate content/error corrections repeat a bin search that ROOT shares. These mechanisms explain plausible overheads, but this timing study does not individually profile their costs.

Sources: [benchmark harness](lookup-benchmark/benchmark.cxx), [lookup adapters](lookup-benchmark/lookup.hxx), [JVT comparison](lookup-benchmark/jvt.cxx). Evidence: [raw systematic measurements](optimization-campaign/28-lookup-comparison-v2/raw.json), [JVT measurements](optimization-campaign/28-lookup-comparison-v2/jvt.json), [summary](optimization-campaign/28-lookup-comparison-v2/summary.json), [provenance](optimization-campaign/28-lookup-comparison-v2/acceptance.json).

Implementation, validation and this report are committed locally in both repositories. Nothing was pushed.
