# CorrectionLib: readable authoring and efficient execution

## Purpose and user intent

Build a human-friendly correction authoring and application interface on top of the existing ATLAS CorrectionLib demonstration. The user particularly likes the interfaces illustrated below. Preserve their simplicity while making their semantics precise.

The goal is a real declarative rewrite: the correction definition is the source of truth for the physics prescription. Athena and columnar analysis execute that definition. Handwritten adapters extract inputs and write outputs; they should not implement component-specific physics formulas.

This is a standalone handoff for an agent with no prior conversation. It describes proposed work, not an API that already exists. Read the existing implementation before designing or changing it.

## What CorrectionLib does

CorrectionLib represents scalar corrections using typed inputs, formulas, binnings, categorical branches, transforms and compositions, distributed as versioned JSON. Its compiled C++ evaluator executes the loaded definitions, with Python bindings for array evaluation.

The pinned version does not generate and compile specialized C++ for each correction graph. Graph evaluation, batch execution, SIMD and JIT compilation are different capabilities. Do not imply that one automatically supplies the others. A specialized compiler is not a prerequisite for this task.

References:

- https://cms-nanoaod.github.io/correctionlib/
- https://cms-nanoaod.github.io/correctionlib/schemav2.html

## The desired architecture

Separate three responsibilities:

1. **Authoring:** physicists describe the prescription using readable, typed expressions and named stages.
2. **Distribution and evaluation:** a build step produces validated CorrectionLib payloads and any explicitly versioned generic extension graphs. Runtime evaluators load these once.
3. **Application:** a small analysis-facing interface resolves a recommendation, prepares inputs, evaluates requested variations and returns named results.

Neither authors nor analysis users should need to hand-assemble deeply nested JSON or manage intermediate scratch registers.

## Authoring interface

An illustrative authoring experience:

```python
response = interpolate(response_histogram, pt, abs(eta))
uncertainty = response * (mu - mu_reference)

pileup_component = correction(
    name="pileup_mu",
    inputs=[pt, eta, mu],
    value=uncertainty,
    validity=valid_jet_region,
)
```

These calls construct a symbolic definition. They do not evaluate event data. The names and signatures are proposals; choose precise APIs after inspecting the current compiler and graph schema.

The authoring layer should:

- Track input types, units, valid ranges and named outputs.
- Make float32 rounding explicit where required by the reference calculation.
- Give shared intermediate expressions stable names and dependencies.
- Compose named stages and reusable subcorrections.
- Validate definitions at build time and report errors using author-facing names.
- Emit deterministic payloads with source-to-correction provenance.
- Permit inspection of the lowered representation without making it the primary authoring surface.

Use a small Python library rather than inventing a new language or an extensive framework. Start with existing working expressions and progressively replace low-level graph construction.

## Reuse without an oversized primitive library

Distinguish an authoring helper from a runtime primitive:

- A reusable authoring function can expand into existing nodes. It needs no evaluator extension.
- A new runtime primitive is justified by a missing numerical operation, required semantics, or demonstrably excessive duplication/recomputation.

Primitives must be generic mathematical or execution operations, not opaque ATLAS-specific corrections. A flavour component should be assembled from table evaluations and arithmetic, rather than implemented by a runtime branch recognizing a flavour-component name.

For example, express the flavour-composition uncertainty as:

```text
delta_fg * abs(response_q - response_g)
-------------------------------------------------
fg * response_g + (1 - fg) * response_q
```

The payload owns this expression, its input table references and its validity prescription. The runtime owns the generic operations needed to execute it.

## Analysis-facing interface

The desired application experience is conceptually:

```python
jets = JetCorrections.load("ATLAS/Run3/EMPFlow/<version>")

result = jets.evaluate(
    jet_columns,
    event_columns,
    systematics="recommended",
)
```

This is a proposed facade, not an existing CorrectionLib API or a currently available recommendation registry. Resolve the placeholder to a real, explicitly pinned local bundle for the first implementation.

Design requirements:

- The user chooses a recommendation bundle and requests outputs, rather than assembling a correction graph.
- Required columns, units and shapes are discoverable before evaluation.
- Event boundaries and original jet identities are preserved.
- Results expose named momenta, scores, decisions, weights and validity/status information.
- Define exactly what happens for missing inputs and out-of-validity values. Never silently discard jets or substitute unity weights.
- "recommended" expands to an inspectable, recorded systematic list. The output metadata records the exact bundle and resolved configuration.
- Support requesting a subset of systematics and streaming results when materializing all variations would be expensive.
- Resolve the same bundle for the Athena adapter. Preserve the existing analysis configuration compatibility requirement.

Choose the precise result object and array conventions through a small working example. Avoid inventing convenience methods in documentation before they exist.

### Worked analysis example: ROOT file to a jet-pT histogram

The following expands the proposed facade into an analysis workflow: open a PHYSLITE ROOT file, read chunks with uproot, evaluate nominal jet corrections, and fill a Hist histogram. **The nominal-calibration slice below is now implemented as C-nice; see the implementation and timing section at the end.** The `atlas_jet_corrections` import, input mapping and named result contract below are implemented for nominal calibration on Path C. Other proposed interfaces remain future work. The ROOT branch names follow the existing nominal runner; the file and bundle paths must be replaced with real local paths.

For this first example, request only `NOSYS` and nominal calibration. Assume the facade evaluates only the requested output stages, so this call needs neither truth-jet inputs nor NNJVT inputs. Each jet input retains its event-by-jet Awkward shape, including any additional per-vertex or per-sampling dimensions. Event inputs retain one entry per event. The facade owns primary-vertex selection, input preparation and collection-shape validation. Inputs must use the original PHYSLITE units; numerical arrays do not carry unit metadata. Its nominal result has matching event-by-jet `pt` (MeV) and boolean `valid` arrays. In C-nice, `valid` checks finite stage outputs; backend exceptions propagate, and this does not establish a broader physics validity domain. Missing required branches or malformed inputs raise an error.

```python
import awkward as ak
import hist
import matplotlib.pyplot as plt
import uproot

from atlas_jet_corrections import JetCorrections  # C-nice package.

# Load once, outside the event-chunk loop. Pin a real local bundle.
jets = JetCorrections.load("/path/to/pinned-run3-empflow-bundle")

# Explicit bindings make the source columns visible. The bundle should
# eventually supply an inspectable, validated PHYSLITE input schema.
jet_fields = [
    "JetConstitScaleMomentum_pt", "JetConstitScaleMomentum_eta",
    "JetConstitScaleMomentum_phi", "JetConstitScaleMomentum_m",
    "DetectorEta", "ActiveArea4vec_pt", "SumPtChargedPFOPt500",
    "EnergyPerSampling", "NumTrkPt1000", "TrackWidthPt1000",
]
event_branches = {
    "vertex_type": "PrimaryVerticesAuxDyn.vertexType",
    "rho": "Kt4EMPFlowNeutEventShapeAuxDyn.Density",
    "mu": "EventInfoAuxDyn.averageInteractionsPerCrossing",
    "event_number": "EventInfoAuxDyn.eventNumber",
}
jet_branches = {name: "AnalysisJetsAuxDyn." + name for name in jet_fields}
branches = list(jet_branches.values()) + list(event_branches.values())

h_pt = hist.Hist.new.Reg(50, 0, 500, name="pt", label="Jet $p_T$ [GeV]").Double()

with uproot.open("/path/to/PHYSLITE.root") as source:
    tree = source["CollectionTree"]
    for columns in tree.iterate(
        expressions=branches, step_size="50 MB", library="ak", how=dict,
    ):
        result = jets.evaluate(
            {name: columns[branch] for name, branch in jet_branches.items()},
            {name: columns[branch] for name, branch in event_branches.items()},
            systematics=["NOSYS"],
            outputs=["calibration"],  # Nominal calibration only.
        )
        nominal = result["NOSYS"]  # Named Awkward result fields.

        # Make the invalid-jet policy explicit for this example.
        if not bool(ak.all(nominal.valid, axis=None)):
            raise RuntimeError("Calibration returned invalid jets; inspect statuses")

        pt_gev = nominal.pt / 1000.0
        eta = columns[jet_branches["JetConstitScaleMomentum_eta"]]
        selected = (pt_gev > 20.0) & (abs(eta) < 2.5)
        h_pt.fill(pt=ak.to_numpy(ak.flatten(pt_gev[selected], axis=1)))

fig, ax = plt.subplots(figsize=(6, 4))
h_pt.plot1d(ax=ax, histtype="step", label="Nominal calibrated jets")
ax.set_ylabel("Jets / 10 GeV")
ax.legend()
fig.tight_layout()
fig.savefig("jet-pt.png", dpi=150)
plt.show()
```

This fills one unweighted entry per selected jet, with pT above 20 GeV and absolute input eta below 2.5. The nominal calibration preserves eta. Values above 500 GeV remain in the histogram overflow; the displayed range is 0–500 GeV. Event weights, JVT selections and systematic comparisons are separate extensions. Selection happens after correction, before flattening, so event boundaries remain available throughout evaluation. Empty jet collections contribute no entries. Chunking limits input memory; `step_size` is an uproot reading target, not a cap on the evaluator's total memory.

The file-reading and plotting portions use the documented [uproot chunk iteration](https://uproot.readthedocs.io/en/stable/basic.html), [Hist construction and filling](https://hist.readthedocs.io/en/stable/user-guide/notebooks/Histogram.html), and [Hist plotting](https://hist.readthedocs.io/en/latest/user-guide/notebooks/Plots.html) APIs. They require uproot, Awkward, Hist and its plotting dependencies (Matplotlib and mplhep), in addition to the C-nice facade. The concrete example has been run in WSL; validation and timings are recorded at the end of this document.

## Efficient execution

Prepare as much as possible once:

1. Load and validate payloads once per evaluator instance or worker.
2. Resolve graph names, typed inputs, model references and systematic selections once.
3. Extract and prepare original event/jet inputs once per chunk.
4. Compute nominal quantities and reusable intermediates once where dependencies permit.
5. Recompute only descendants affected by each variation.

Caching must follow declared dependencies. For example, truth matching based on original jet directions can be shared, while NNJVT quantities depending on varied momentum must be recalculated for affected variations. Never reuse a nominal result merely because its name is unchanged.

Reduce Python calls in inner loops, array-to-list conversions, temporary allocations, duplicated payload loading and duplicated subexpression evaluation. Prefer array inputs with explicit shape contracts. Do not confuse array acceptance with vectorized machine-code execution.

First profile initialization, input reading, feature preparation, numerical evaluation, output writing and memory separately. Optimize a measured bottleneck while maintaining the numerical contract. Keep diagnostic reference comparisons outside performance measurements when reporting evaluator or application throughput.

## Human inspection and debugging

Provide coherent inspection capabilities; exact API names can be chosen during implementation:

- **Describe:** stages, dependencies, required inputs, units, validity rules and systematic coverage.
- **Explain:** trace one selected jet through the actual graph, including intermediate values and branches taken.
- **Compare:** identify the first divergent stage against a reference, with inputs and expected/observed results.
- **Provenance:** identify source settings, constants, histogram objects, model versions and hashes.

Generate traces and descriptions from executable definitions where possible. Do not maintain a parallel handwritten description of the physics that can drift from the payload.

## Existing implementation: do not start from scratch

All builds and execution belong in WSL2 distro **atlas_al9**, under:

```text
/home/gwatts/atlas/correctionlib_tests
```

Do not build or run from the Windows-mounted filesystem. This Windows chat workspace contains inspection copies:

```text
C:\Users\gwatts\OneDrive\Documents\ATLAS\Computing\AMG\CorrectionLib
```

The demonstration has four paths:

- **A:** untouched AnalysisBase, the numerical authority.
- **B:** rebuilt ATLAS tools using the original shared CorrectionLib backend and unchanged analysis YAML.
- **C:** original PHYSLITE columns through that shared backend.
- **D:** a subsequently completed declarative implementation, isolated in `path-d/`.

Preserve the validated implementations, environments and outputs. Develop the authoring/application interface in a separate directory or package, building on D rather than replacing it blindly.

### Path D's current capabilities

Path D provides a private optional `correction::GraphSet` extension, with its own graph schema version 1 and evaluator requirement. It is not standard CorrectionLib schema v2. The existing interpolation tables and GSC composition remain ordinary schema-v2 payloads.

The extension supports named intermediate values, formulas, calls to ordinary corrections and compounds, reusable multi-output graph calls, lazy branches, assertions, explicit casts, ROOT coordinate operations, TRandom3 Gaussian sampling and generic lwtnn inference.

Its payloads define component formulas, stage ordering, systematic branches, rounding, random widths/seeds, NNJVT features/cuts, scale factors and validity decisions. It includes 119 complete pipeline graphs. The batch implementation still loops over scalar rows; it is not JIT, SIMD or GPU execution. Collection extraction and reductions remain Python.

This work supplies a substantial runtime foundation. The next task is principally to make authoring readable and application efficient and elegant, while evaluating whether the existing extension boundaries need refinement.

Read these WSL files first:

```text
README.md
docs/calculation-inventory.md
docs/implementation-log.md
path-d/README.md
path-d/docs/design.md
path-d/python/compile_graphs.py
path-d/python/schema.py
path-d/results/validation-summary.md
```

Windows inspection copies of D are under `path-d-inspection/`. Treat WSL sources as authoritative. Inspect the code rather than relying solely on the summaries.

## Numerical contract and evidence

Pinned reference: AnalysisBase 25.2.100, x86_64-el9-gcc15-opt; CorrectionLib v2.7.0 base plus required full-precision JSON parsing patch. D has an additional isolated graph extension. Preserve both patches where applicable.

The input is the Run-3 FullSim ttbar PHYSLITE sample resolved by the release's `TestFileLiteR3MC.txt`: 20,000 events and 196,606 jets, AnalysisJets / AntiKt4EMPFlow.

Scope: nominal calibration, Category JES / Full JER, pseudo-data JER and NNJVT FixedEffPt selection and efficiency weights. There are 117 kinematic states and two additional weight-only states, giving 119 output states. This is not the All/All uncertainty model. Large-R jets, flavour-tagging weights, forward JVT and MET propagation are excluded.

Keep these tolerances:

| Quantity | Requirement |
|---|---|
| Momentum/mass in MeV | abs(delta) <= 0.01 + 2e-6 * abs(reference) |
| Eta and wrapped phi | abs(delta) <= 2e-6 |
| Scores and weights | abs(delta) <= 1e-7 + 2e-6 * abs(reference) |
| Identities, counts, coverage and decisions | Exact agreement |

Do not relax tolerances to accommodate a rewrite. Explicit rounding and operation ordering matter. Preserve ATLAS random sampling rather than substituting CorrectionLib's different random generator.

The completed D agent reports full numerical acceptance, exact nominal intermediate coordinates, and full-sample ordering/chunk/worker invariance. Independent probes cover 110 JES, 672 JVT, 8,541 JES/JER and 108 truth-matching cases. Recheck the saved reports before citing these results.

Important qualification: the original full-sample reference does not record per-jet JES/JER statuses. Synthetic original-tool status checks pass; do not claim full-sample JES/JER status parity from unavailable reference data. Athena validation so far is build/initialization, not a full numerical comparison.

D's stronger tests found negative-scale ROOT mass normalization and nonpositive-NPV sentinel behavior omitted by the earlier backend. Preserve those fixes.

## Suggested implementation sequence

1. Audit D's compiler, schema and adapters. Map existing operations to the proposed authoring and analysis interfaces.
2. Implement a small typed expression builder and demonstrate one real component end to end, including provenance and readable validation errors.
3. Express nominal calibration, systematic components and NNJVT/weight stages through that builder, preserving deterministic lowered payloads or documenting equivalent changes.
4. Implement the recommendation facade with explicit input/result contracts and recorded systematic resolution.
5. Add describe/explain support using the same graph representation.
6. Profile the complete workflow and improve chunking, array transfer and reuse based on actual dependencies.
7. Run primitive tests, independent edge probes and full A-versus-new-path acceptance. Check ordering, chunks and worker counts.
8. Produce concise documentation, a runnable example and clearly scoped before/after performance measurements.

Avoid a speculative framework rewrite. A usable vertical slice should demonstrate the proposed interface before converting everything.

## Completion criteria

- A physicist can read and edit the prescription without manipulating JSON/register machinery.
- An analysis user can load the frozen recommendation and evaluate original columns with a small, documented call.
- The lowered graph owns component-specific physics; runtime primitives remain reusable and independently tested.
- Errors, validity, units and systematic selection have explicit semantics.
- Provenance and a one-jet explanation are available from the executed definition.
- Performance measurements include preparation costs and clearly separate validation overhead.
- The existing numerical contract and reference independence are maintained.
- Remaining external operations and unsupported configurations are documented honestly.

## Delivery expectations

Save implementation, tests and reproducible scripts in WSL. Copy the readable interface examples and reports into the Windows chat workspace for remote inspection. Do not overwrite previous acceptance artifacts or claim a new API is implemented merely because its design is documented.

## C-nice implementation and measured results (23 October 2026)

**Nominal recalibration is included.** Path C and both C-nice versions (the initial Python-loop version and the array-processing version) start from PHYSLITE's `JetConstitScaleMomentum_*` columns and rerun pileup, JES and GSC calibration through the shared B/C backend. `NOSYS` means nominal calibration without systematic variations; it does not mean bypassing calibration. In particular, the 8.05-second nominal plotting result includes this recalibration as well as reading, histogram filling and PNG output. Its smaller scope relative to the full workflow, rather than reuse of stored calibrated momentum, explains the distinction between the two timing rows.

For PHYSLITE analyses that can use the stored calibrated jet momentum, a separate facade mode should skip nominal recalibration and apply any requested uncertainties to that stored momentum. That uncertainty-evaluation mode remains unimplemented. A standalone stored-momentum read-and-plot baseline, with no calibration or uncertainties, is now measured below. Future campaigns should explicitly record both choices: whether nominal calibration is rerun or stored calibrated momentum is reused, and whether systematics are enabled. Validate the stored momentum's calibration convention and required uncertainty inputs before enabling reuse with uncertainties.

**Array-processing update:** C-nice now prepares inputs entirely with Awkward/NumPy operations. Primary-vertex selection and per-vertex gathers, event-to-jet broadcasting, energy/fraction calculations and input-matrix construction contain no Python loops over events or jets. Loops over the fixed input-field names and over file chunks remain. The unchanged pybind11/C++ batch evaluator still converts and evaluates rows internally. All 20,000 events / 196,606 jets have bitwise-identical prepared inputs and all 12 stage coordinates compared with the previous validated C-nice run. Empty chunks, zero-jet events and chunk invariance also pass. Details are saved in `c-nice/array-validation.json`. The timing table below has been remeasured using this array-processing implementation. A future campaign will measure both nominal-only and systematic-enabled workflows.

The worked nominal-calibration example is now implemented in WSL at `/home/gwatts/atlas/correctionlib_tests/c-nice`. Windows inspection copies and the generated plot are in `c-nice/` beside this document. Load the existing `payloads/` directory as the bundle. The package requires `c-nice` and `install/python` on `PYTHONPATH` after sourcing the existing `setup.sh`. Run `c-nice/plot-jets.py` for the concrete file-to-histogram example. Plot dependencies installed: Hist 2.12.0 and mplhep 0.4.1.

The facade loads the existing nominal manifest and shared `_atlas_corrections.Nominal` evaluator once, validates input names and collection lengths, prepares the same 14 inputs with the same float rounding as Path C, and returns named Awkward momentum and validity arrays preserving event boundaries. Empty chunks and zero-jet events are supported. Missing or malformed columns and unsupported requests raise errors. As in C, a nonempty vertex collection without a primary vertex uses index zero; an empty vertex collection with jets is explicitly rejected. `valid` means finite stage outputs with backend exceptions propagated; it is not an independently established physics-validity flag.

No C++ physics, payload, binding or Path B adapter was modified. B continues to use exactly the shared backend used by this facade. Consequently B was not remeasured. The new facade currently supports only `systematics=["NOSYS"]` and `outputs=["calibration"]`; the broader recommended-systematics facade remains future work. The complete C-nice diagnostic workflow uses the new facade for nominal calibration followed by the existing C systematic and JVT runners.

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

All runs process 20,000 events / 196,606 jets. Each new workflow has four runs with the first excluded; raw times and commands for the array implementation are saved in `c-nice/array-timing/summary.json` (the earlier measurements remain in `c-nice/summary.json`). Environment setup is outside timing. Full C-nice includes initialization, input preparation, saved intermediate/output files and reference comparisons, and retains C’s repeated kinematic evaluation in the JVT stage. A/B reference comparisons were outside their timers. The plotting timing includes startup, nominal evaluation, histogram filling and PNG output. A/B are historical runs, not interleaved measurements: these numbers do not isolate facade overhead or establish a controlled speedup. The new measurements ran sequentially, with no plotting or dependency installation overlapping the full-workflow runs. The earlier pre-array means were 188.53 s for the full workflow and 28.45 s for the plot; these are separate campaigns, not an interleaved overhead study.

Validation: all four full workflows pass nominal, systematic and JVT comparisons against the original ATLAS reference. Additional interface checks establish bitwise equality between whole and chunked evaluation on 30 events, empty-chunk and zero-jet behavior, and rejection of missing columns, inconsistent shapes and unsupported systematic requests. Existing numerical tolerances are unchanged. The initial empty-chunk test exposed an untyped empty counts array; the facade now constructs integer counts explicitly. No new B numerical claim is made beyond its existing acceptance evidence.

### Stored-momentum read-and-plot baseline

**Path A counterpart:** an explicit no-correction CPRun configuration is saved in `a-stored-timing/a-stored.yaml`. In pinned AnalysisBase 25.2.100, `runSystematics: false` alone does not disable nominal recalibration: `recalibratePhyslite` defaults to true, and `runUncertainties` is also independently enabled by default. The benchmark explicitly sets all three false, disables JVT updates/selections/efficiencies, truth tagging and ghost-muon association, and requests only event number and stored jet pT/eta output. Disabling uncertainties requires `CommonServices.enableExpertMode: true` in this release. This is a diagnostic I/O configuration, not a recommended analysis prescription.

The standard A framework job takes **8.75 ± 0.37 s**, including startup, xAOD input handling, the standard shallow-copy/output machinery, and writing a small ROOT tree. Plotting that tree with Hist takes **0.83 ± 0.07 s**, giving **9.58 ± 0.37 s** end to end (the total SD is measured from per-run totals). Four runs processed 20,000 events / 196,606 jets, with the first discarded; setup and independent validation are outside timing. The retained total times are 9.93, 9.19 and 9.62 s. Per-run output pT/eta and event identities match the original PHYSLITE values exactly, and all histogram bins including flow agree with the direct columnar baseline. No calibration or uncertainty tools are requested. Raw records, configuration, plotting source and logs are in `a-stored-timing/`.

This A measurement uses its established CPRun-to-tree workflow followed by a separate plot, whereas the 1.08-second columnar baseline fills Hist directly from PHYSLITE. It therefore includes intermediate tree writing and rereading, and should not be presented as an isolated comparison of xAOD versus uproot read speed or as a direct in-framework A histogram benchmark. A minimal A algorithm filling the histogram directly would be a further matched-workflow measurement.

The standalone `c-nice/plot-stored-jets.py` reads `AnalysisJetsAuxDyn.pt` and `.eta`, uses their stored values without applying any correction, selects pT > 20 GeV and |eta| < 2.5, and fills the same 50-bin, 0–500 GeV Hist plot. It imports no correction backend and loads no payload. All event/jet work uses arrays inside a 50 MB chunk loop. This is a read/plot baseline, not implementation of stored-momentum uncertainty evaluation in the facade. The stored momenta may differ from the recalibrated momenta: equality with the C-nice recalibrated histogram is not assumed.

The second mode reads the original 14 C-nice input branches in addition to stored pT/eta, but does not prepare calibration features or evaluate corrections. It measures the cost of reading more branches; its input set contains two more branches than the recalibration example. Modes ran alternately, four times each, with the first run of each excluded. Both modes and all repetitions yield identical histogram counts, including underflow/overflow, with 20,000 events and 196,606 jets. Timers include startup/imports, reading, selection, histogram filling, PNG output and the small count-report JSON; setup is excluded. No cache flushing was performed. The earlier 8.05-second recalibration result is from a separate campaign. Results and logs: `c-nice/stored-timing/summary.json`.
