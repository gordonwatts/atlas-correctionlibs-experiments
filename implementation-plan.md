# ATLAS jet corrections with CorrectionLib: implementation plan

## Objective and fixed scope
All work runs inside atlas_al9 WSL2 at /home/gwatts/atlas/correctionlib_tests.
Compare A (untouched AnalysisBase, authoritative), B (same AnalysisBase configuration with rebuilt CorrectionLib-backed tools), and C (original PHYSLITE read as columns into a shared evaluator).
Pin AnalysisBase 25.2.100 x86_64-el9-gcc15-opt and source-built CorrectionLib v2.7.0. Record resolved commits and dependencies.
Use Run-3 FullSim ttbar dataset 601229, p7266, DAOD_PHYSLITE.49568713._001693.pool.root.1, from the release TestFileLiteR3MC.txt (20,000 events). Use AnalysisJets/AntiKt4EMPFlowJets.
Cover nominal recalibration, every Category JES / Full JER variation, pseudo-data JER, NNJVT selection and efficiency/inefficiency/event weights.
The inspected Run-3 payload has no All/All configuration. Exclude large-R, flavour-tagging weights, forward JVT and MET propagation.

## Reference and inventory
Run unmodified AnalysisBase on 100 events, freeze a working configuration, then use all 20,000 for acceptance.
Enable recalibration, NNJVT updating and FixedEffPt weights. PHYSLITE stores NNJvtPass rather than NNJvt; reconstruct the score with the original network and retained features.
For every active calculation record input variables, units, precision, formulas, source files/objects, precedence, defaults, validity rules and intermediate outputs.
Area/residual pile-up: text eta boundaries and piecewise-linear alpha/beta coefficients; offsetMeV = 1000*(alpha*(muCorr-muRef)+beta*(npvCorr-npvRef)); ptAfter = pt-rho*area-offset. Reset NEGATIVE results to 10 MeV, not small positive results.
JES: etaJes TList of TH1 response histograms and reciprocal TH1::Interpolate, with low-energy extrapolation/high-energy freezing. The selected mode named spline is not cubic spline evaluation.
Eta correction: text coefficients of a log-energy polynomial, energy limits, direction update and four-vector reconstruction.
GSC: charged fraction, Tile0, EM3, track count and track width in sequence, using eta-selected TH2 bilinear responses.
JES uncertainties: histograms, pile-up/flavour dependencies, referenced functions and linear/quadrature group combinations.
JER: nominal resolution and uncertainty histograms, sign rules, TRandom3 smearing.
NNJVT: network JSON and cut JSON. JVT weights: histogram contents/errors, passing and failing factors.
Trace PathResolver versioned paths, TEnv merges, defaults and named ROOT objects. Record checksums and source identity.

## Payload compiler and shared evaluator
Implement an offline compiler following the complete dependency chain and TEnv precedence.
Export consumed constants, axes, contents/errors, functions, interpolation modes and boundaries to schema-v2 CorrectionLib JSON plus a separate provenance/configuration manifest. Map generated corrections to source objects. Reject unsupported inputs, never approximate silently.
Original calibration ROOT files must not be runtime dependencies of the replacement numerical evaluator.
Use Category for labels/branches; Binning/MultiBinning for regions and true constant lookups; Formula/FormulaRef for interpolation/arithmetic; Transform for input transformations; CompoundCorrection for ordered sequences.
GSC consists of five inverse-response corrections with inputs_update=["pt"], input_op="*", output_op="*". Each stage sees updated pt. Each stage handles eta selection, special values, boundaries and bilinear interpolation. Flatten compound sequences because v2.7.0 stacks reference ordinary corrections, not compound corrections.
Do not reduce direction-changing calibration to a single scale factor.
Expose scalar and batch C++ entry points with explicit jet/event inputs, resolved config and systematic ID, returning momenta, decorations, weights and statuses. No Athena/StoreGate/xAOD types in these interfaces.
Shared layer preserves units, float rounding, starting scales, vertex/truth matching, standalone lwtnn inference, four-vector reconstruction, event reductions and ATLAS TRandom3 behavior. Preserve seeds including zero-phi fallback, MC/pseudo-data differences, Gaussian/sign rules and rejection of negative draws. Do not substitute HashPRNG.
Each variation starts from nominal. Preserve linear/quadrature uncertainty groups and SF_fail=(1-SF_pass*eff)/(1-eff), including simultaneous histogram variations. Use immutable payloads and per-call/worker mutable state.
If an unsupported scalar operation requires a CorrectionLib extension, update schema, C++ dispatch, serialization and tests together in a pinned fork; mark evaluator requirements and prepare an upstreamable patch.

## Integration
Build CorrectionLib and bindings with the AnalysisBase compiler/Python into a private workspace installation; verify C++/Python agreement.
Rebuild affected ATLAS packages preserving tool/component names, properties, interfaces, systematic registration, decorations and return codes. Existing properties resolve converted bundles. Select backend through separate build environments with identical YAML/Python. Never load competing component implementations together; fail unsupported configs explicitly.
Retain Athena adapters and perform build/initialization smoke testing in a compatible pinned Athena release; AnalysisBase remains the numerical authority.
Columnar runner reads original PHYSLITE with Uproot/Awkward, preserving offsets, nested vertex moments and original jet order; broadcasts event inputs, independently computes features/truth matching and calls shared batch bindings. Reconstruct jagged outputs/event weights.
Use an exported reference configuration manifest, not duplicated recommendation logic. An input-only xAOD exporter may aid diagnosis but cannot replace direct file reading in final acceptance or supply reference-calibrated outputs.
Document PHYS configuration and smoke-test its corresponding source/preprocessing; do not demand equality between independently produced formats.

## Milestones and acceptance
1. Working reference and complete inventory.
2. Residual, JES interpolation and five-stage GSC prototype against original stage implementations.
3. Complete selected uncertainty/JVT evaluator.
4. Path B equivalence with unchanged config.
5. Path C equivalence on full input.
Key records by file identity, entry, original jet index and systematic. Record before thinning/sorting; retain intermediate states, final momenta, NNJVT decisions/scores, weights and statuses.
Tolerance: momentum/mass in MeV abs(delta)<=0.01+2e-6*abs(reference); angles <=2e-6 with wrapped phi; scores/weights <=1e-7+2e-6*abs(reference). Exact IDs, coverage, decisions, statuses and counts. Never automatically relax thresholds.
Test bin centres/edges, extrapolation/freezing, negative momentum, GSC special inputs, flavour conditions, JVT thresholds/pass/fail, MC/pseudo-data and zero-phi seeds, missing/invalid inputs; add synthetic edge cases.
Repeat varying chunk sizes, ordering and workers. Compare A-B and A-C, additionally B-C, all variations and all 20,000 events. Diagnose first divergent stage.
Deliver build/run scripts, frozen config/manifest, compiler, bundle, shared evaluator, adapters, columnar runner, machine-readable report and source traceability. Report per-systematic maxima/mismatch counts.
Completion requires all required variations passing against untouched AnalysisBase. Only then benchmark startup, I/O, evaluation and memory separately.
