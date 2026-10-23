# Implementation log: from the approved plan to the working demo

This is a retrospective, high-level account of the implementation, checked against the source on 22 October 2026. It describes the logical build sequence; it is not a timestamped commit history. All work took place in atlas_al9 under /home/gwatts/atlas/correctionlib_tests.

## 1. Establish the unchanged reference

Pinned AnalysisBase 25.2.100 and the Run-3 FullSim PHYSLITE test file. Configured recalibration, Category JES / Full JER, pseudo-data JER and NNJVT FixedEffPt. Saved the configuration and its checksum. The reference job records intermediate jet momenta, final variations, NNJVT scores/selections and jet/event weights before thinning or sorting.

This gave the replacement a numerical target rather than relying on a description of the recommended corrections. The selected model supplies 117 nominal/kinematic states, with two additional NNJVT weight variations.

Evidence: config/reference.yaml, config/reference.frozen.yaml, scripts/run-reference.sh.

## 2. Trace the original calculation and its constants

Read the calibration, uncertainty, jet-labeling and JVT implementation in the pinned release. Followed PathResolver paths, ROOT TEnv settings and referenced ROOT objects. Identified the actual interpolation and arithmetic, including details that are easy to miss:

- The selected JES “spline” path uses histograms in the etaJes TList and TH1 interpolation.
- Nominal pile-up and uncertainty evaluation use different definitions of the vertex count.
- GSC changes the momentum after each of its five stages.
- This configuration uses calibrated eta for NNJVT selection/efficiency; DetectorEta is used elsewhere in calibration.
- Truth matching is based on original jet directions, before recalibration.

Evidence: docs/calculation-inventory.md; python/atlas_correctionlib/payloads.py.

## 3. Build a reproducible private CorrectionLib environment

Built the pinned CorrectionLib source with the AnalysisBase toolchain, alongside a private Python environment and pybind11 bindings. Recorded release/compiler/package versions. Because the release Python's SSL library did not match the host, wheel downloads use verified HTTPS through the distro Python, followed by offline installation into the release Python environment.

Evidence: scripts/bootstrap.sh, scripts/download-dependencies.py, scripts/freeze-environment.py, config/requirements-wheels.lock, CMakeLists.txt.

## 4. Compile ATLAS payloads into CorrectionLib functions

Implemented compilers that load original calibration data and write schema-v2 JSON plus provenance manifests. Exported residual coefficients, JES and eta corrections, GSC response surfaces, uncertainty tables, and JVT histogram contents/errors.

Histogram binning selects interpolation cells; formulas reproduce the interpolation within each cell. Shared FormulaRef expressions reduce repeated formula storage. The five GSC inverse responses are composed using CompoundCorrection, updating pt between stages.

The compilers are specific to the pinned bundle. They are not a generic converter for arbitrary ATLAS configurations. Unsupported selected modes are rejected, while unused objects in source files can remain inventoried without conversion.

Evidence: python/atlas_correctionlib/{compile_nominal.py,compile_uncertainties.py,nodes.py}; payloads/*manifest.json.

## 5. Implement the shared numerical layer

Added C++ classes for nominal calibration, JES/JER variations, NNJVT evaluation and JVT efficiency. Inputs are explicit numerical values rather than xAOD or StoreGate objects.

The layer preserves stage-by-stage float rounding, unit conversions, negative pile-up handling, eta-dependent four-vector reconstruction and the source TRandom3 seed/draw behavior. Each systematic starts from nominal momentum. For the selected correlated uncertainty groups, component contributions add linearly; other correlation modes are rejected.

NNJVT uses standalone lwtnn and the original model. JVT efficiency retains pass/fail formulas and source float arithmetic. Python bindings expose the same C++ calculations to the columnar path.

Evidence: src/backend.hxx, src/systematics.hxx, src/nnjvt.hxx, src/efficiency.hxx, src/nominal.cxx. An earlier Python uncertainty prototype remains in python/atlas_correctionlib/systematics.py, but the active Systematics class calls C++.

## 6. Resolve the precision discrepancy without loosening tolerances

Validation exposed rare near-zero mass differences. The investigation traced these to decimal constants changing by one double-precision bit in CorrectionLib's default RapidJSON parser. Enabled full-precision parsing for string, plain-file and compressed-file entry points, rebuilt CorrectionLib, and added a regression test.

After this patch, all nominal intermediate coordinates match the authoritative output exactly on the full sample. No custom schema node or relaxed numerical tolerance was required. The patch and upstream explanation are saved locally; no upstream submission was made.

Evidence: patches/correctionlib-full-precision-json.patch, docs/correctionlib-upstream-note.md, tests/, results/nominal-comparison.json.

## 7. Replace the numerical internals of selected ATLAS tools

Generated local source overlays for JetCalibTools, JetUncertainties and JetJvtEfficiency. The adapters keep the established component names and the same analysis YAML, prepare framework inputs and call the shared backend. They reject configurations outside the explicitly supported bundle.

The replacement nominal tool writes the three intermediate calibration momenta. The uncertainty adapter registers the supported nuisance parameters and handles individual up/down variations. JVT retains the framework's truth/validity wrappers and replaces its numerical lookups. The standard NNJVT update tool remains in the ATLAS path, using the bundled original network.

Reference and replacement run in separate processes with separate library setups. The CVMFS release stays untouched.

Evidence: scripts/prepare-overlay.py, src/adapter_support.hxx, scripts/build-overlay.sh, setup-replacement.sh.

## 8. Reproduce the calculation from original PHYSLITE columns

Implemented three scripts: nominal calibration, systematic momenta, and NNJVT/weights. They read original columns through Uproot/Awkward and reconstruct vertex-dependent features and truth matching. The nominal script writes an intermediate cache consumed by the following scripts; reference results are used for comparisons, not as numerical inputs.

The present workflow favors diagnosis: it repeats systematic evaluation in the JVT stage, loads some full-sample arrays, converts arrays into C++ vector inputs, and writes separate intermediate/output files. It is not yet a fused, memory-bounded, optimized columnar runner. Its environment still uses the AnalysisBase toolchain, ROOT and lwtnn.

Evidence: scripts/run-columnar-{nominal,systematics,jvt}.py; python/atlas_correctionlib/{systematics,jvt}.py.

## 9. Validate the three paths and package runtime data

Compared A/B and A/C over all 20,000 events and 196,606 jets, with B/C as an additional check. Added original-tool JES and JVT boundary probes, interpolation/precision/smearing tests, and full-sample determinism tests with shuffled order, different chunks and one/four workers.

Packaged converted tables, source settings and NNJVT files with hashes. The restricted-DATAPATH smoke test exercises the replacement bundle; no system-call file-open audit was performed.

Also compared reference/replacement PHYS on 100 events. Built the adapters with Athena 25.0.76 and initialized five tools with zero events. Athena is an integration smoke test, not an independent numerical reference.

Evidence: scripts/package-bundle.py, scripts/check-bundle-only.sh, scripts/run-phys-smoke.sh, scripts/build-athena.sh, scripts/run-athena-smoke.sh, scripts/summarize-validation.py; results/validation-summary.md.

## 10. Measure the implemented workflows

The original performance report measured components on 1,000 events and excluded feature reconstruction. The new repeated timing exercise runs each complete existing workflow four times on 20,000 events, discards the first run, and reports the mean and sample standard deviation of runs 2–4.

Environment setup is outside the timer; process startup, input preparation, computation and output are inside. The columnar workflow also performs reference comparisons and repeats systematic calculations. ATLAS output comparisons happen after its timed job. Consequently these are workflow timings, not an isolated comparison of evaluator speed.

Evidence: scripts/repeated-timing.py; results/repeated-timing/summary.md and summary.json. Every run has separate outputs/logs, and is checked against the reference.

## What the source review qualifies

- This implementation validates one frozen recommendation bundle, not every property or specialist query API exposed by the original tools.
- The JES/JER shared evaluator returns success for completed evaluations; its columnar writer records that success. The reference comparison does not establish general per-jet JES/JER validity-status parity outside this sample. JVT has explicit validity handling and the independent boundary test compares its status codes.
- The active uncertainty wrapper checks the shared status before yielding momenta, but it does not retain detailed failure records.
- Python orchestration still prepares features, performs truth matching and reduces event weights. Those operations have not all been moved into the framework-independent C++ layer.
- The interface is batch-capable, but the C++ batch methods currently loop over scalar evaluations; this is not a SIMD or GPU implementation.
- Some compiler manifest status labels describe the conversion milestone, not final validation; results/validation-summary.json records the subsequent acceptance evidence.
- No numerical algorithms were changed for the repeated timing exercise.

## Files checked for this account

Reviewed the compiler/node builders, all four shared C++ calculation headers and bindings, adapter generator, active Python wrappers, all three columnar runners, build/setup scripts, precision patch, existing calculation inventory and acceptance-report generator. This review establishes how the current demo is assembled; it is not a claim of exhaustive production-readiness review.
