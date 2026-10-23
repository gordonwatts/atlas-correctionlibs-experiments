# Path D: declarative jet corrections

Path D is isolated under path-d/, with a private copy of the pinned CorrectionLib source in external/correctionlib-d. A/B/C source, environments, binaries and validation outputs remain unchanged. The original RapidJSON full-precision patch is preserved. Build and execution occur only in WSL2 atlas_al9.

## What is declarative

python/compile_graphs.py emits versioned, inspectable payload graphs. These contain residual combinations, area subtraction, nominal feature arithmetic, JES/eta four-vector stage sequencing, GSC order, each component formula, correlated sums, flavour branches, mu/NPV dependencies, JES/JER direction and pseudo-data choices, widths/seeds/fallback seeds, NNJVT features and right-closed bins, selection, efficiency/inefficiency formulas and float rounding, truth-match geometry and threshold, and validity/status decisions.

There are 117 kinematic graphs and 119 complete pipeline graphs. Complete pipelines call nominal calibration, the selected systematic, and NNJVT/weights through reusable multi-output graph calls. The diagnostic runners also expose stages independently to locate first divergence. The manifest owns state names, weight-only mappings and shifts; runtime adapters do not parse component names or implement component formulas.

The converted interpolation tables and five-stage GSC compound retain schema-v2 Binning, Formula, FormulaRef, Transform and CompoundCorrection. Table provenance remains linked to the original nominal and uncertainty manifests, including source ROOT objects and configuration hashes.

## Generic extension and why it exists

This is an optional correction::GraphSet extension around the existing CorrectionLib evaluator, with companion graph schema version 1 and requirement correctionlib-2.7.0+graph-1. It does not pretend its new operations are standard schema-v2 nodes. The schema-v2 correction set remains valid and separately loadable. external/correctionlib-d/include/correction_graph.h implements the parser/evaluator; python/schema.py defines strict Pydantic serialization and exports schemas/graph-v1.json.

Existing schema-v2 scalar nodes cannot conveniently expose named shared intermediates, return several stage coordinates, call another multi-output program, explicitly round to binary32, or reproduce ROOT's random-number stream. CompoundCorrection updates scalar inputs and reduces scalar outputs; it is insufficient for this workflow. The extension therefore adds:

| Operation | Generic meaning |
|---|---|
| constant, formula, call | Named registers, existing TFormula evaluation and schema-v2 correction/compound calls |
| graph | Reusable multi-output graph call; dependencies resolve before use and cycles fail |
| if, assert | Lazy numerical branching and explicit domain errors |
| cast | Explicit float32 rounding or checked truncation to int64 |
| coordinates | Named ROOT coordinate conversions/properties and coordinate normalization |
| gaussian | Explicit nonzero uint32 seed, mean, width and lower truncation, using ROOT TRandom3::Gaus |
| inference | Model-relative lwtnn FastGraph<double> call with payload-defined feature names |

These operations contain no ATLAS tool, jet, systematic or component names. ROOT coordinate normalization is intentionally explicit: negative mass in PtEtaPhiM coordinates has special behavior, exposed by the independent 10-MeV JES test. Random widths, signs, seeds and zero-phi fallback policies are formulas/branches in the payload. HashPRNG is not substituted. Gaussian rejection has a one-million-draw hard failure bound; accepted ATLAS cases are far below it.

Graph schema v1 accepts finite real-valued inputs and numeric registers. String/categorical dispatch is represented by named graph selection; this extension does not claim to cover every possible standard CorrectionLib input type. Its inference primitive supports a declared scalar lwtnn input node and one selected output per call.

The evaluator interprets loaded graphs. It does not generate specialized C++, JIT code, SIMD kernels or GPU code. Batch calls loop over scalar rows, release the Python GIL and use local mutable state.

## Explicit boundaries

ROOT's coordinate definitions, including conversion arithmetic and negative-mass normalization, are reusable numerical primitives. lwtnn is an explicit model-inference boundary, not a hidden NNJVT tool: bins, RpT, score rounding, cuts, selection and SF calculations are graphs. The network JSON is bundled.

Adapters select the primary vertex, extract retained columns, count vertices using the two source definitions, enumerate jet/truth pairs, and perform ordinary any/product reductions by jet/event. The truth-pair distance and threshold live in a graph. Event products include only graph-valid jet weights. These collection-assembly and reduction operations remain Python; this is not an end-to-end standalone C++ event reader.

The selected JES/JER configuration has no global/component validity histograms. Original component code returns true in their absence, so its graph status is success. The compiler rejects newly configured validity histograms until explicitly converted. Independent synthetic original-tool checks test statuses; the pre-existing full-sample reference does not contain per-jet JES/JER statuses, so full-sample status parity is not claimed. JVT validity is explicit and has independent original-tool boundary/status coverage.

Nonpositive uncertainty NPV uses the original -1234 pile-up-weight sentinel. Nonfinite input and invalid graph/domain conditions fail explicitly. This is one frozen recommendation bundle, not a replacement for every original tool property or specialist API.

## Validation and reproducibility

Run bash path-d/scripts/run.sh from the WSL workspace. It builds the private evaluator, compiles/validates the schema, runs the three original-input diagnostic stages, independent original-tool boundary probes, generic primitive/serialization tests, pipeline-composition tests, and ordering/chunk/worker tests. All results go under path-d/results/.

No reference-calibrated result is a physics input. Nominal rows are independently assembled from the original PHYSLITE file; subsequent stages use D's own nominal cache and additional original columns. The authority is untouched AnalysisBase A. The tolerances are unchanged: momentum/mass 0.01 + 2e-6*abs(reference) MeV; angles 2e-6; scores/weights 1e-7 + 2e-6*abs(reference); identities, counts, selections and recorded status checks are exact.

source-traceability.json records original implementation hashes and graph-to-source associations. patches/correctionlib-graph-v1.patch captures the optional fork extension. The original precision patch is separately retained in the workspace. Runtime provenance and the final numerical report are generated from actual completed checks, rather than implied by compiler metadata.

No benchmark is an isolated comparison of CorrectionLib against hand-written physics: the diagnostic workflow includes I/O, Python preparation, repeated kinematic evaluation, output and reference comparisons. Timing is reported only after correctness, with that scope stated.
