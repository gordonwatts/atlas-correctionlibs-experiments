# 2006-10-24 - Talk Outline

Draft for discussion and editing. Prepared 24 October 2026; the **2006** in the document title is retained exactly as requested. This is the outline and source handoff, not a slide deck.

## Talk intent and working assumptions

Audience: ATLAS colleagues familiar with jets, calibration tools and systematics, but not necessarily CorrectionLib or columnar Python. Assume roughly 30 minutes, with 23 main slides and technical backup. Shorten by combining slides 3–4, 11–12 and 20–21; keep detailed measurement caveats in notes rather than dropping them.

Suggested talk title: **Trying CorrectionLib for ATLAS jet corrections**.

Opening voice: “I've wanted to try CorrectionLib for a long time. This finally gave me an excuse to take a real calibration chain, make it work in a columnar analysis, and see how fast we could make it.”

The story: start with an untouched ATLAS reference; extract the same calibration constants; implement a shared backend that runs inside the existing framework and behind a columnar interface; validate it; optimize one change at a time. Most improvements came from avoiding repeated work and data movement. CorrectionLib supplies useful calibration-data infrastructure, but its lookup engine was not intrinsically faster than the original ROOT lookup in our measurements.

### Instructions for the later slide-building chat

- Build the deck only after this outline has been edited/approved. Do not infer approval of a visual theme, template or destination from this file.
- Treat the tables below and their linked raw evidence as the numerical source. Older report headings sometimes say “selected” for historical checkpoints; [the complete history](x2-optimization.md) and [uniform-bin follow-up](uniform-binning.md) resolve the current state.
- Preserve the sequence requested: motivation → CorrectionLib → A → columnar C-nice → B integration and first comparison → optimization → final comparison → backup.
- Put a small source reference and checkpoint on result slides. Put full links, implementation hashes and caveats in speaker notes. The complete optimization table already contains full authoritative Git hashes.
- Label every abbreviated snippet either **actual code excerpt** or **simplified illustration**. The toy payload below is invented teaching data, not an ATLAS recommendation. Proposed interfaces in old design documents must not be presented as implemented.
- Main slides should have one point each. Code should normally fit in 8–15 lines; move field lists, input paths and full YAML to backup. Use the real jet-pT plot if useful, but note its nominal-only scope.
- Use diagram arrows to explain A/B/C and the export pipeline. Do not imply that the C++ backend is SIMD/vectorized or that CorrectionLib JIT-compiles a bespoke kernel.
- Authoritative execution is WSL `atlas_al9:/home/gwatts/atlas/correctionlib_tests`. This Windows repository contains reports and inspection copies. Relative links here refer to the Windows copies. Original ROOT outputs stay in WSL. No new benchmark is requested by this outline.

## Main talk

### 1. I've wanted to try this for a while

**Point:** This is a concrete experiment with a real calibration chain, not just a toy CorrectionLib example.

**On slide:** Motivation in the opening wording above; “Can we use the same calibration in our existing software and in a columnar analysis?”

**Visual:** Three small boxes, initially without implementation detail: reference / framework replacement / columnar application.

**Say:** We can use the existing software as an oracle and keep the calibration prescription tied to something we can check. The scope is a frozen MC23 jet setup, not every ATLAS calibration or data-taking condition.

**Sources:** [Implementation record](implementation-log.md), [validation summary](validation-summary.md), [complete timing history](x2-optimization.md).

### 2. What would count as a useful result?

**Point:** A cleaner application interface is only useful if the physics and existing use case still work.

**On slide:** Same constants; agreement with A; B configuration unchanged; readable C-nice code; single-thread optimization; one local commit per experiment.

**Say:** The accepted scope permitted differences up to random-seed choices, but the implemented RNG optimization actually preserved ROOT's stream. Validation against A uses explicit tolerances; several optimization-to-optimization comparisons are stricter byte-level checks. Do not say all A/B/C quantities are bitwise identical.

**Sources:** [RNG study](single-thread-profiling.md), [NN acceptance](nn-optimization.md), [latest parity](optimization-results/29-uniform-binning/parity.json).

### 3. CorrectionLib: a payload and an evaluator

**Point:** A correction is a function with named, typed inputs and a numerical output, distributed as data.

**On slide:** `inputs → category/binning/formula/transform → value`; JSON parsed at loading; compiled C++ evaluator, Python bindings.

**Say:** A bin may hold a constant, another node or a formula. Compound corrections can apply stages in sequence. The evaluator executes a graph; it does not generate and compile a specialized C++ function for each payload. Separately, our adapters organize arrays and batches.

**Visual:** A small two-level node diagram, not the entire generated payload.

**Sources:** [Architecture discussion, “What CorrectionLib does”](correctionlib-interface-handoff.md), [actual node builder](columnar-source/python/atlas_correctionlib/nodes.py). That discussion contains later Path D proposals: use its introductory explanation only here.

### 4. A tiny payload, and the code that calls it

**Point:** Show the audience the whole contract with a small readable example.

**On slide — simplified teaching payload:**

```json
{
  "schema_version": 2,
  "corrections": [{
    "name": "toy_scale", "version": 1,
    "inputs": [{"name": "pt", "type": "real"}],
    "output": {"name": "scale", "type": "real"},
    "data": {"nodetype": "binning", "input": "pt",
      "edges": [0.0, 50.0, 100.0], "content": [1.02, 1.01],
      "flow": "clamp"}
  }]
}
```

```python
import correctionlib
corrections = correctionlib.CorrectionSet.from_file("toy.json")
scale = corrections["toy_scale"].evaluate(75.0)  # 1.01
```

**Say:** pT is in GeV by this toy's convention; CorrectionLib's real input type does not itself enforce units. The example returns a scale; our code decides how to apply it. Actual ATLAS exports are larger and include interpolation and formulas.

**Deck-builder check:** This toy is independent of the real payload. Keep real-valued edges written as JSON floating-point numbers; the pinned parser rejects integer edge literals. Revalidate if the example is edited for the deck.

### 5. What did we put into CorrectionLib?

**Point:** We reformatted existing constants; we did not fit new calibrations.

**Visual:** Original ATLAS config and ROOT calibration files → exporter → CorrectionLib JSON plus manifests → shared B/C backend.

**On slide:** Nominal pileup/JES/GSC response functions; uncertainty/resolution tables; JVT efficiency and error tables. Keep NNJVT network and RNG in a separate box: lwtnn/Eigen and ROOT.

**Say:** The payload is a frozen snapshot of the selected recommendation, not a live conditions-system connection. Our C++ still implements ordering, four-vector updates, special uncertainty combinations, selection, validity and rounding. B and C do not constitute a fully declarative rewrite of the physics prescription. That was the separate Path D experiment and is not this talk's main result.

**Sources:** [Nominal backend](columnar-source/src/backend.hxx), [systematics](columnar-source/src/systematics.hxx), [efficiency](columnar-source/src/efficiency.hxx), [NN wrapper](columnar-source/src/nnjvt.hxx), [lookup study](lookup-comparison.md). Original exporters in WSL: `python/atlas_correctionlib/compile_nominal.py` and `compile_uncertainties.py`.

### 6. Path A: run the software we already have

**Point:** The reference is the untouched release running a real sample.

**On slide:** AnalysisBase **25.2.100**, WSL `atlas_al9`, x86_64 EL9 / GCC 15; AntiKt4EMPFlow jets; MC23 ttbar PHYSLITE; **20,000 events / 196,606 jets**; nominal chain plus JES/JER and NNJVT/weights; **117 kinematic states / 119 JVT/weight states** in the comparison workflow.

**Actual configuration excerpt:**

```yaml
CommonServices:
  runSystematics: true
Jets:
  - containerName: AnaJets
    jetCollection: AntiKt4EMPFlowJets
    recalibratePhyslite: true
    runNNJvtUpdate: true
    systematicsModelJES: Category
    systematicsModelJER: Full
```

**Result:** **237.26 ± 0.96 s** for A's full job, historical four-run campaign, first excluded. Includes process startup, tool initialization, event processing and output; environment setup is excluded and reference comparison is outside A's timer.

**Notes:** “Untouched” refers to the A release/tools. We deliberately configure nominal recalibration for validation. This does not mean normal PHYSLITE analysis needs it. Show only the compact sample identifier on screen; full path is in backup.

**Sources:** [A/B configuration](talk-outline-sources/reference.yaml), [four-path timings](four-path-timing-comparison.md).

### 7. How fast can it be if we just read and plot?

**Point:** Separate the framework/I/O floor from calibration and systematics.

| Workflow | Seconds | What is included |
|---|---:|---|
| A, full configured job | 237.26 ± 0.96 | Recalibration and full variations |
| A, stored jets → tree | 8.75 ± 0.37 | Calibration/JVT/uncertainties explicitly disabled |
| A, stored jets → tree → Hist plot | 9.58 ± 0.37 | Includes intermediate-tree I/O |
| Columnar stored pT/eta → Hist | 1.08 ± 0.05 | Reads two branches; no calibration |
| Same columnar plot, also read calibration inputs | 2.22 ± 0.02 | Additional input-I/O control |
| C-nice nominal recalibration → Hist | 8.05 ± 0.06 | Nominal-only plot, not full diagnostic workflow |

**Say:** Turning systematics off alone does not disable nominal calibration. For the stored-jet baseline we explicitly turn off recalibration, uncertainties and JVT work. The original framework floor is already quite low, including its startup.

**Do not invent a missing row:** We do not have a matched A nominal-recalibration-only measurement here. If desired for the final talk, it is a separate future measurement. The recorded C-nice nominal-only plot is not a substitute for that A measurement.

**Visual:** Compact table; optional thumbnail of the real jet-pT histogram. Keep “different output scopes” in the slide footer.

**Sources:** [Stored-jet configuration](a-stored-timing/a-stored.yaml), [A measurements](a-stored-timing/summary.json), [columnar stored-jet measurements](c-nice/stored-timing/summary.json), [facade scope](c-nice/README.md), [plot](c-nice/array-timing/jet-pt.png).

### 8. Path C-nice: what I want to write

**Point:** The analysis sees arrays and named results, with a loop only over chunks.

**Simplified excerpt from the working nominal plotting example** (imports and explicit field bindings on backup):

```python
jets = JetCorrections.load(bundle)
h = hist.Hist.new.Reg(50, 0, 500, name="pt").Double()
with uproot.open(filename) as source:
    for columns in source["CollectionTree"].iterate(
        expressions=branches, step_size="50 MB", library="ak", how=dict
    ):
        result = jets.evaluate(
            {n: columns[b] for n, b in jet_branches.items()},
            {n: columns[b] for n, b in event_branches.items()},
            systematics=["NOSYS"], outputs=["calibration"])
        nominal = result["NOSYS"]
        if not bool(ak.all(nominal.valid, axis=None)):
            raise RuntimeError("Invalid calibration result")
        pt = nominal.pt / 1000.0
        selected = (pt > 20) & (abs(nominal.eta) < 2.5)
        h.fill(pt=ak.to_numpy(ak.flatten(pt[selected])))
```

**Notes:** This illustration selects using corrected eta; the saved full script currently uses its input eta for that plotting cut. Do not call the fragment a verbatim reproduction. `h.plot1d()` provides the plot. The facade currently supports only `NOSYS` and calibration outputs. Full-systematics timing combines the facade's nominal stage with the C systematics/JVT workflow. Do not show a facade API that requests all variations as if it exists.

**Sources:** [Complete working script](c-nice/plot-jets.py), [facade](c-nice/atlas_jet_corrections.py), [full diagnostic runner](columnar-source/scripts/run-columnar-full.py), [README](c-nice/README.md).

### 9. Columnar outside, shared physics inside

**Point:** We did not have to rewrite every C++ correction as a vector kernel.

**Visual:** Uproot → Awkward/NumPy input columns → native batch adapter → shared scalar physics → array results → Hist.

**Say:** No Python loops over jets or events in the analysis input preparation. C++ still iterates through rows. Field-name loops and chunk loops are fine. The plot example uses 50 MB chunks; the full benchmark uses 1,000-event chunks. Do not substitute one for the other in reproductions.

**Sources:** [Array adapter](columnar-source/src/nominal.cxx), [array validation](c-nice/array-validation.json), [full runner](columnar-source/scripts/run-columnar-full.py).

### 10. Path B: put the same backend into the old release

**Point:** The analysis configuration stays the same; the implementation behind the tools changes.

**Visual:** Shared JSON/backend at center; B adapters on one side expose existing ATLAS tool interfaces; C array adapters on the other expose Python arrays.

**Actual setup excerpt:**

```bash
source setup.sh
source build-overlay/x86_64-el9-gcc15-opt/setup.sh
export ATLAS_CORRECTIONLIB_BUNDLE="$PWD/payloads"
export DATAPATH="$ATLAS_CORRECTIONLIB_BUNDLE:${DATAPATH:-}"
```

**Say:** The CVMFS reference remains untouched. A locally built package overlay replaces the relevant tool implementations and retains their public properties/input handling. The validated setup covers a pinned set of properties and rejects unsupported configurations. Later NN optimization also added the JetMomentTools overlay. We checked actual loaded library paths, not just that the job ran.

**Sources:** [Frozen setup copy](talk-outline-sources/setup-replacement.sh), [same A/B YAML](talk-outline-sources/reference.yaml), [B loaded libraries](optimization-results/27-final-B-timing/run-2/loaded-libraries.txt), [NN integration](nn-optimization.md). Historical WSL generator: `scripts/prepare-overlay.py`; do not rerun it just to build slides, as it recreates overlay sources and predates some later changes.

### 11. First comparison: all three on one page

**Point:** The replacement works in both contexts; columnar execution is already useful before the optimization campaign.

| Path | Historical full workflow | Role |
|---|---:|---|
| A — untouched ATLAS | 237.26 ± 0.96 s | Numerical reference |
| B — ATLAS with shared replacement | 290.70 ± 1.01 s | Same analysis configuration |
| C-nice — array facade plus C stages | 161.89 ± 3.55 s | Initial array version |

**Notes:** These are recorded historical workflow measurements, not contemporaneous equal-affinity tests. A/B comparisons are outside their timers; C comparisons are inside. The later fresh optimization baseline is **155.86 ± 0.51 s**, a different run of the starting workflow; do not silently replace 161.89 with 155.86 in this initial comparison.

**Sources:** [Four-path report](four-path-timing-comparison.md), [array-facade timing](c-nice/array-timing/summary.json), [complete history](x2-optimization.md).

### 12. Optimization rules: how fast can we make it?

**Point:** Make each change reviewable and preserve the physics.

**On slide:** One change → validate → time → commit. Keep B working; keep B's config and the C-nice interface stable; one CPU; allow memory/CPU tradeoffs; retain outputs and comparisons.

**Say:** We briefly tested four native workers, then returned to the single-thread objective. Steps 05–08 are a clearly marked side branch in the performance story. No need to hide unsuccessful experiments: NN cache alone was inconclusive; larger chunks were reverted; uniform encoding did not accelerate this sample.

**Visual:** Main single-CPU trajectory **56.27 → 49.63 → 37.88 → 31.71 → 26.44 → 25.61 s**, followed by the **24.95 s latest repeat**, with an annotation that the last difference is not a uniform-bin gain. Full checkpoint list in backup.

### 13. Big gain: cross the Python/C++ boundary in arrays

**Point:** Millions of individual values are expensive to transfer through Python containers.

**Historical change:** **155.86 → 112.06 s** for native dense-array bindings. Later contiguous-output/input-row work: **30.35 → 28.48 → 26.44 s**.

**Simplified conceptual comparison:**

```python
# Earlier transfer shape; illustrative, not a current API recommendation.
result = native.batch(rows.tolist())
# Dense array boundary; actual adapter family.
result = native.batch_array(rows)
```

**Show native excerpt:** `input.data()`, `output.mutable_data()`, one reusable row, native loop. Explain that removing temporary containers preserves scalar physics; it is not a SIMD claim. Distinguish the initial binding change from later output-allocation work.

**Sources:** [Bindings](columnar-source/src/nominal.cxx), checkpoints 01, 19, 20 in [history](x2-optimization.md); historical versions via their Git commits.

### 14. Big gain: stop doing the same calculation twice

**Point:** Workflow reuse can matter more than tuning a single lookup.

**Two examples:** reuse systematic kinematics in JVT, **100.79 → 83.21 s**; combine stages in one process and share loaded arrays/tools, **34.90 → 31.71 s**.

**Simplified illustration:**

```python
kinematics = evaluate_systematics(inputs)
validate_and_save(kinematics)
jvt = evaluate_jvt(kinematics)  # use the already computed states
```

**Say:** These are explanatory names, not a facade API. Actual reuse checks payload/backend provenance, event IDs, jet ordering and state names. The diagnostic outputs and comparisons remain enabled. More retained arrays trade memory for time; peak RSS was not measured.

**Sources:** [JVT runner](columnar-source/scripts/run-columnar-jvt.py), [integrated runner](columnar-source/scripts/run-columnar-full.py), [facade workflow notes](c-nice/README.md).

### 15. Big gain: resolve metadata and tables once

**Point:** Loading a description should not be part of each jet's calculation.

**Changes:** Metadata parsed at construction, **108.25 → 100.79 s**; bind named correction objects once, **42.97 → 37.88 s**.

**Simplified C++ illustration:**

```cpp
// Constructor: resolve the name once.
auto response = tables->at(correction_name);
// Evaluation: use the bound object.
double value = response->evaluate({pt, eta});
```

**Say:** The metadata and bound-table changes happened at different checkpoints. The source still needs physics-specific recipes; the improvement moves interpretation out of the hot loop.

**Sources:** [Compiled component and Table definitions](columnar-source/src/systematics.hxx), checkpoints 03 and 14.

### 16. Big gain: one unwanted vector copy

**Point:** Profiling found an avoidable cost inside CorrectionLib itself.

**Actual patch:**

```cpp
- const auto bins = std::get<_NonUniformBins>(bins_);
+ const auto& bins = std::get<_NonUniformBins>(bins_);
```

**Result:** **47.13 → 42.97 s**. The search remains `std::upper_bound`; the edge vector is no longer copied on each lookup.

**Say:** The prior **49.63 → 47.13 s** shift was a rebuild control, not this patch's gain. Full-precision JSON parsing is a separate pre-existing patch. Do not present this as a claim about every current upstream CorrectionLib version.

**Sources:** [Bin-copy patch](vendor-patches/bin-edge-reference.patch), [precision patch](vendor-patches/existing-full-precision-json.patch), [history](x2-optimization.md).

### 17. Reuse the common part of systematic variations

**Point:** Up/down and related resolution states share calculations.

**Result:** **37.88 → 34.90 s**.

**Actual pattern, shortened from `batch_group_into`:**

```cpp
const auto prepared = amounts(variations.front(), row);
for (const auto& variation : variations) {
    const auto result = apply(variation, prepared, row);
    // Write this state's result into its output slice.
}
```

**Say:** The grouped C path and scalar B path still call the same preparation/application arithmetic. We do not assume arbitrary variations share everything; grouping follows the recipe.

**Sources:** [Grouped systematic evaluator](columnar-source/src/systematics.hxx), checkpoint 15 and its acceptance in [history](x2-optimization.md).

### 18. RNG and NN: optimize the work around the arithmetic

**Point:** Small kernels can spend substantial effort setting up or allocating.

**RNG:** Final-seed construction and exact Gaussian cache, **56.27 → 54.08 → 49.63 s**. Keep the original stream and width-dependent rejection. Show seed-tagged cache hit/miss pseudocode; full implementation in backup.

**NN:** Original detailed native study attributed **41.23% of sampled NN instructions** to allocation/free. Cache identical four-feature tuples; reuse intermediate Eigen buffers. **26.44 ± 0.42 → 25.61 ± 0.31 s** combined; cache alone **27.01 ± 1.00 s**, inconclusive.

**Say:** The cache avoids an evaluation on a hit. Reusable buffers remove Eigen heap allocations after warm-up on the supported network path. Neither eliminates all allocations in the full program. No precision reduction was adopted.

**Sources:** [RNG report](single-thread-profiling.md), [smearing code](columnar-source/src/backend.hxx), [native profile](calculation-profile-report.md), [NN report](nn-optimization.md), [buffers](columnar-source/src/nn_buffered.hxx). Split this slide into two if time permits.

### 19. What did careful profiling tell us?

**Point:** Once the calculation is cheap, the diagnostic workflow matters.

**Historical checkpoint-20 scoped timings:** reference reads **7.53 s**; JVT wrapper **4.42 s**; tool loading **3.75 s**; ROOT writes **2.55 s**; grouped systematics **1.63 s**; nominal preparation/evaluation **0.71 s**; remainder **7.03 s**, total **27.62 s** diagnostic run.

**Visual:** Scoped elapsed-time bar, explicitly labeled checkpoint 20. Optional inset: native NN **3.145 s**, efficiency **0.595 s**, systematics **1.594 s** from the separate higher-resolution study.

**Say:** Do not add the inset to the outer bar: scopes overlap and runs differ. Instruction shares are not CPU-time shares. These profiles predate the NN changes; a fresh final profile has not been captured. Validation-reference reading is not a normal production-analysis requirement, but removing it would change our benchmark.

**Sources:** [Whole-workflow profile](final-profile-report.md), [calculation profile](calculation-profile-report.md).

### 20. Is CorrectionLib's lookup faster than ROOT's?

**Point:** We measured it rather than assuming it.

| Lookup | Original ATLAS/ROOT | CorrectionLib |
|---|---:|---:|
| 1D interpolation | 33.4 ns | 40.8 ns |
| 2D interpolation | 104.9 ns | 130.1 ns |
| pT-only interpolation in 2D | 57.2 ns | 61.8 ns |
| JVT scale factor + error | 24.8 ns | 54.2 ns |

**Application test:** swap only lookup in shared systematic replay: **1.115 ± 0.012 s** original, **1.307 ± 0.074 s** CorrectionLib. All 117 output states numerically identical on the replay sample.

**Say:** These are warmed native measurements, not A/B workflow times. ROOT shares the content/error bin search; our CorrectionLib encoding evaluates two corrections. Reusing argument vectors helps, but remains a benchmark-only experiment. CorrectionLib's value here is portable calibration payloads and a common evaluator, not an observed lookup-speed advantage.

**Sources:** [Lookup report](lookup-comparison.md), [matched harness](lookup-benchmark/benchmark.cxx), [JVT harness](lookup-benchmark/jvt.cxx).

### 21. A useful negative result: uniform bins

**Point:** A faster representation must still select the right bin at the edges.

**Show:** Explicit edges use binary search; uniform bins use arithmetic. On `[-4.4, 0, 4.4]`, the double immediately below zero can enter a different bin because of rounding.

**Result:** Only **12 inactive nodes** qualified for conservative exact conversion. Fresh control **24.95 ± 0.02 s**, candidate **24.95 ± 0.08 s**. **No speedup.**

**Say:** Keep this short; it illustrates why validation is part of each step. The latest 24.95 s figure is not evidence that uniform binning improved the preceding 25.61 s measurement.

**Sources:** [Uniform-bin study](uniform-binning.md), [export guard](columnar-source/python/atlas_correctionlib/nodes.py), [audit evidence](optimization-results/29-uniform-binning/axis-audit.json).

### 22. Final results: A, B and C-nice

**Point:** The shared backend remains usable in the existing framework, and the columnar diagnostic workflow is much faster.

| Path | Earlier full workflow | Latest repeated measurement | Interpretation |
|---|---:|---:|---|
| A, untouched reference | 237.26 ± 0.96 s | Same historical reference | Not rerun at the end |
| B, existing framework + shared backend | 290.70 ± 1.01 s | **257.80 ± 4.10 s** | Latest repeated B test, checkpoint 27 |
| C-nice + full C systematic/JVT workflow | 161.89 ± 3.55 s | **24.95 ± 0.08 s** | Latest full run, checkpoint 29 |

**Required notes:** Latest B single validation after the uniform change was **265.05 s** and passed; do not replace the repeated mean with it or invent its uncertainty. B's repeated checkpoint uses the same active lookup paths as checkpoint 29. A/B reference comparisons are outside their timers; C's are inside. A and early B affinity policy differs from the pinned final runs. These are implemented-workflow measurements, not a controlled measurement of CorrectionLib speedup.

**Say:** B's repeated result was 11.3% below its historical baseline. For the strict single-CPU C campaign, 56.27 s became roughly 25 s. The attempt to halve 49.63 s came close; don't claim a precise achieved 2× based on run variation. All full comparisons passed.

**Visual:** Table or grouped bars, with measurement scope immediately beneath. Avoid a giant A/C speedup badge that conceals the scope differences.

**Sources:** [Complete history](x2-optimization.md), [repeated final B](optimization-results/27-final-B-timing/summary.json), [latest C](optimization-results/29-uniform-binning/summary.json), [latest B validation](optimization-results/29-uniform-binning/B-integration/summary.json).

### 23. What did we learn, and what is next?

**Point:** The experiment gives us a working interface, shared physics and a reviewable optimization history.

**On slide:** Reuse existing calibration data; keep the reference executable; move data in arrays; avoid repeated interpretation, initialization and allocation; measure instead of guessing.

**Next steps, explicitly future work:** broader samples and recommendations; a matched systematics-on/off campaign with stored nominal jets versus recalibration made explicit; memory measurements; fresh profiling of the final implementation; evaluate the next individual changes for experiment acceptance.

**Say:** The clean full-systematics facade is not yet implemented. Shared C++ plus CorrectionLib already lets us test the columnar direction without requiring a complete rewrite of the scalar physics. The local commits give us small pieces that can be reviewed independently; nothing has been pushed or proposed as an upstream merge by this study.

## Backup slides

Each backup should contain a concrete example, its numerical scope and a link; do not fill backup with tables alone.

| Backup | Content / example to show | Evidence |
|---|---|---|
| B01: full sample and reproduction | Exact path below; `CPRun.py -i "$sample" -t config/reference.yaml -o output.root -e 20000`; setup A versus overlay B | [YAML](talk-outline-sources/reference.yaml), [setup](talk-outline-sources/setup-replacement.sh), [B command records](optimization-results/27-final-B-timing/summary.json) |
| B02: full plotting example | Imports, jet/event field bindings, `uproot.open`, chunk iteration, validity, selection, Hist fill and save; show actual file rather than proposed all-systematics API | [plot-jets.py](c-nice/plot-jets.py), [stored-jet plot](c-nice/plot-stored-jets.py), [README](c-nice/README.md) |
| B03: all checkpoint timings | Complete table below, split over two slides; full Git hashes in notes | [history](x2-optimization.md), [earlier campaign](c-nice-optimization.md) |
| B04: array event-weight products | Illustrate `ak.prod(weights, axis=1)` with the actual float64 conversion and empty-event policy; **112.06 → 108.25 s** | [JVT runner](columnar-source/scripts/run-columnar-jvt.py), checkpoint 02 |
| B05: JVT input arrays | Show primary-vertex indexing and truth-match array expressions, not a nested Python jet loop; **61.05 → 54.41 s** in four-worker interval | [JVT runner](columnar-source/scripts/run-columnar-jvt.py), checkpoint 06; note pair-array memory cost |
| B06: output compression | Actual `uproot.LZ4(...)` setting from the runner; same columns, different compression; **54.41 → 43.26 s**, four workers | [systematics runner](columnar-source/scripts/run-columnar-systematics.py), checkpoint 07 |
| B07: lazy metadata | Show optional inspection data loaded on first access rather than tool construction; **43.26 → 38.72 s**, four workers | [JVT Python wrapper](columnar-source/python/atlas_correctionlib/jvt.py), checkpoint 08 |
| B08: reference reads | Show reading bounded groups of 16 branches rather than each branch independently; **31.71 → 30.45 s** | [reference reader](columnar-source/python/atlas_correctionlib/reference.py), checkpoint 17 |
| B09: exact JVT cache | Input equality and invalidation examples: changed values, sigma, tool, mutated returned arrays; **30.45 → 30.35 ± 0.99 s**, inconclusive | [JVT wrapper](columnar-source/python/atlas_correctionlib/jvt.py), [cache checks](x2-check-jvt-cache.py) |
| B10: RNG details | Seed-tagged first Gaussian cache, then original-stream fallback on rejection; explain correlations and why caching an accepted draw would be wrong | [backend](columnar-source/src/backend.hxx), [RNG report](single-thread-profiling.md) |
| B11: NN cache versus buffers | Four exact feature bits, collision/model/thread isolation; 4→8→5→3→1 network; one reusable vector per operation; original fallback | [NN report](nn-optimization.md), [cache](columnar-source/src/nn_graph.hxx), [buffers](columnar-source/src/nn_buffered.hxx) |
| B12: no-gain and reverted experiments | Larger chunks **26.64 ± 0.41 s**, reverted; feature cache alone **27.01 ± 1.00 s**; uniform encoding unchanged | [history](x2-optimization.md), [NN report](nn-optimization.md), [uniform report](uniform-binning.md) |
| B13: CorrectionLib representation | Constant bin → nested bin → formula; linear/bilinear export; default/clamp/error; uniform guard example | [node builder](columnar-source/python/atlas_correctionlib/nodes.py), [uniform report](uniform-binning.md) |
| B14: lookup study details | Reused-argument column, 58 table/mode objects, actual original library, alternated order, boundary checks; no whole-job inference | [lookup report](lookup-comparison.md), [raw results](optimization-results/28-lookup-comparison-v2/raw.json) |
| B15: validation and limits | A tolerances versus byte-level checkpoint parity; statuses, identities, state coverage; single sample, release, MC only | [validation](validation-summary.md), [parity](optimization-results/29-uniform-binning/parity.json), [NN acceptance](nn-optimization.md) |
| B16: timing method and scope | First excluded, short samples, SD not uncertainty on mean, warm caches, affinity change, validation inclusion; no matched A nominal-only row | [four-path report](four-path-timing-comparison.md), [latest study](uniform-binning.md) |
| B17: what was Path D? | Only if asked: separate declarative graph experiment, not the implementation behind C-nice; historical **283.21 ± 22.37 s**, different experiment | [D design](path-d-inspection/docs/design.md), [D timing](path-d-inspection/results/timing.md) |
| B18: B overhead context | Why a shared lookup backend does not remove xAOD/framework traversal; optional earlier kernel/scaling work, labeled historical | [A/B overhead study](ab-overhead-study.md), [earlier code review](columnar-correctionlib-review.md) |

### Full optimization checklist for backup and speaker notes

Use full source links and hashes from [x2-optimization.md](x2-optimization.md). Adjacent differences are checkpoint observations, not isolated causal estimates. The four-worker section must be shaded or separately boxed.

| Step | What changed | C-nice seconds | Placement |
|---|---|---:|---|
| 00 | Fresh baseline | 155.86 ± 0.51 | Main 13 / B03 |
| 01 | Dense array bindings | 112.06 ± 0.42 | Main 13 |
| 02 | Array event products | 108.25 ± 0.77 | B04 |
| 03 | Compile metadata once | 100.79 ± 0.33 | Main 15 |
| 04 | Reuse kinematics | 83.21 ± 0.88 | Main 14 |
| 05 | Four native workers | 61.05 ± 0.09 | B03, historical parallel experiment |
| 06 | Array JVT inputs, four workers | 54.41 ± 0.68 | B05 |
| 07 | LZ4 output, four workers | 43.26 ± 0.10 | B06 |
| 08 | Lazy metadata, four workers | 38.72 ± 0.29 | B07 |
| 09 | One CPU / one worker | 56.27 ± 0.03 | Main 12 |
| 10 | Avoid redundant RNG seeding | 54.08 ± 0.42 | Main 18 / B10 |
| 11 | Exact Gaussian reuse | 49.63 ± 0.39 | Main 18 / B10 |
| 12 | Release rebuild control | 47.13 ± 0.03 | Main 16 notes |
| 13 | Reference bin edges, no vector copy | 42.97 ± 0.00 | Main 16; displayed zero SD is rounding |
| 14 | Bind tables once | 37.88 ± 0.55 | Main 15 |
| 15 | Group related systematic calculations | 34.90 ± 0.04 | Main 17 |
| 16 | Shared in-process workflow | 31.71 ± 0.14 | Main 14 |
| 17 | Batched reference reads | 30.45 ± 0.19 | B08 |
| 18 | Exact JVT reuse | 30.35 ± 0.99 | B09, inconclusive |
| 19 | Flat grouped output | 28.48 ± 0.12 | Main 13 |
| 20 | Reuse input rows / flat adapters | 26.44 ± 0.42 | Main 13 |
| 21 | Larger chunks | 26.64 ± 0.41 | B12, reverted |
| 22–24 | Profiling studies | Not optimization timings | Main 19 |
| 25 | Exact NN feature cache | 27.01 ± 1.00 | Main 18 / B11, inconclusive alone |
| 26 | Reusable NN intermediates | 25.61 ± 0.31 | Main 18 / B11 |
| 27 | Repeated final B timing | B: 257.80 ± 4.10 | Main 22 |
| 28 | ATLAS/CorrectionLib lookup comparison | Native study, not C-nice checkpoint | Main 20 |
| 29 | Conservative uniform encoding | 24.95 ± 0.08 | Main 21; control 24.95 ± 0.02, no gain |

## Reproduction and evidence index

### Frozen sample

The file is the AnalysisBase `TestFileLiteR3MC.txt` sample used by the recorded commands. Full path:

```text
/cvmfs/atlas-nightlies.cern.ch/repo/data/data-art/ASG/DAOD_PHYSLITE/p7266/mc23_13p6TeV.601229.PhPy8EG_A14_ttbar_hdamp258p75_SingleLep.deriv.DAOD_PHYSLITE.e8514_e8528_s4162_s4114_r15540_r15516_p7266/DAOD_PHYSLITE.49568713._001693.pool.root.1
```

A/B use `config/reference.yaml`; its unchanged SHA-256 is `4d84aa237b6a1796c05e9a38fa341e89993f1b8c582bc99da7b1624b4ed46ca8`. The [inspection copy](talk-outline-sources/reference.yaml) is provided specifically for slide preparation. Full no-calibration config is [separate](a-stored-timing/a-stored.yaml): do not replace the A/B comparison config with it.

### Report map for another chat

| Document | Use in this talk | Caution |
|---|---|---|
| [Complete history](x2-optimization.md) | Primary timing table, all implementation hashes, links to later studies | Primary authority for current checkpoint |
| [Four-path comparison](four-path-timing-comparison.md) | A/B initial means, first C/D context, no-calibration scope | Historical results, not equal-affinity campaign |
| [Original C-nice optimization](c-nice-optimization.md) | Early changes 00–11 | Four-worker interval explicitly labeled |
| [Single-thread/RNG study](single-thread-profiling.md) | Exact RNG optimizations and early profile | Historical stage |
| [Final workflow profile](final-profile-report.md) | Outer timings for checkpoint 20 | “Final” in filename predates NN work |
| [Detailed calculation profile](calculation-profile-report.md) | NN allocation/instruction evidence and proposals | Proposed cache/buffers were implemented later |
| [NN optimization](nn-optimization.md) | Cache, buffers, tests, shared B integration | Use later timing reports for current result |
| [Lookup comparison](lookup-comparison.md) | Original lookup versus CorrectionLib | Microbenchmark and replay, not workflow speedup |
| [Uniform-bin study](uniform-binning.md) | Latest checkpoint, fresh control, rejected unsafe axes | No active lookup speedup |
| [C-nice README](c-nice/README.md) | Implemented API and exact scope | Contains historical timing updates |
| [Interface handoff](correctionlib-interface-handoff.md) | Motivation/design background and earlier plotting discussion | Contains proposed authoring APIs and Path D ambitions |
| [Validation summary](validation-summary.md) | Original numerical coverage and acceptance | Supplement with later per-run reports |
| [Implementation log](implementation-log.md) | Original chronology / extraction and integration context | Not latest timing authority |
| [A/B overhead study](ab-overhead-study.md) | Optional framework/scalar overhead background | Different stages and studies |
| [Columnar review](columnar-correctionlib-review.md) | Optional architectural discussion | Check historical findings against current code |
| [Git snapshot](GIT-SNAPSHOT.md) | Earlier preservation point | Later commits supersede its current-state claims |
| [D documentation](path-d-inspection/README.md) | Backup only, if asked | Different path; do not attribute its design to C-nice |

### Final review checklist before building slides

- Keep the requested filename/title date as-is unless the user edits it.
- Confirm intended talk duration; default is about 30 minutes, not a commitment to a final slide count.
- Mark which code is working, shortened, illustrative or proposed. Do not show nonexistent full-systematics facade calls.
- Distinguish stored nominal jets, explicit nominal recalibration and systematic variations throughout.
- Carry all timing-scope footnotes into the deck. Do not call the early A/B numbers matched one-CPU measurements.
- Keep B's repeated 257.80 ± 4.10 s separate from its latest 265.05 s single validation run.
- Do not credit the 25.61 → 24.95 difference to uniform encoding; the fresh control was also 24.95 s.
- Do not turn instruction percentages into wall-time fractions or treat the checkpoint-20 profile as a newly measured final profile.
- Put the entire experiment history and source hashes in backup/notes so individual changes remain reviewable.
- If material is shortened, preserve the main results and limitations rather than claiming missing measurements were made.
