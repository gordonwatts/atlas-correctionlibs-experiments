# Conservative uniform-bin encoding — checkpoint 29

Uniform encoding is enabled where the installed CorrectionLib arithmetic preserves finite-input bin membership. The updated C-nice measurement is **24.95 ± 0.08 s**, compared with a fresh unchanged control of **24.95 ± 0.02 s**. **No speedup is established:** only 12 inactive lookup nodes qualified for conversion in this payload.

| Full-systematics C-nice | First run, excluded | Run 2 | Run 3 | Mean ± sample SD |
|---|---:|---:|---:|---:|
| Fresh checkpoint-26 control | 25.888 | 24.963 | 24.928 | 24.945 ± 0.025 |
| Checkpoint 29: uniform encoding | 25.345 | 24.894 | 25.013 | 24.953 ± 0.084 |

Same 20,000 events, 196,606 jets, 1,000-event chunks, one CPU and one native/BLAS/OpenMP worker. Startup, nominal recalibration, full systematic/JVT processing, outputs and all A comparisons remain timed. Three sequential repetitions per version; discard the first. The control precedes the candidate, without cache flushing or interleaving. Earlier checkpoint 26 remains recorded at 25.61 ± 0.31 s; use the fresh control when assessing this change.

## Why the conversion is limited

Equal spacing does not guarantee equivalent bin selection with the installed evaluator. Explicit edges use `upper_bound`; uniform edges use `n * ((x-low)/(high-low))` followed by integer conversion. Subtraction, division or multiplication can round a value immediately below an edge onto the edge. For example, with edges `[-4.4, 0, 4.4]`, the representable double immediately below zero belongs to bin 0 in the explicit representation but the uniform arithmetic selects bin 1. Some axes also round an in-range point just below the upper limit to index n.

| Audit classification | Distinct axes | Binning nodes |
|---|---:|---:|
| boundary-rounding | 15 | 2773 |
| nonfinite | 15 | 2486 |
| nonuniform | 41 | 14529 |
| safe | 1 | 12 |

The one eligible axis is `[15, 7000]`, a single bin. Its 12 occurrences are the non-interpolated (`None`) gluon-fraction and gluon-fraction-error tables for six jet definitions. The selected systematic backend uses the `Full` interpolation versions, so its active lookup paths are unchanged. Nominal and JVT-efficiency payload paths receive no uniform replacement. In particular, this experiment has not accelerated the hot interpolated tables from the preceding lookup study.

## Implementation and validation

- The shared export helper checks approximate equal spacing, then checks each exact edge and its adjacent representable doubles using the installed evaluator arithmetic order. Monotonic indexing between those transitions provides the finite-input bin-membership criterion. Axes with infinite edges, nonuniform spacing or unsafe transitions remain explicit. Wrap flow also remains explicit. This criterion depends on the current arithmetic implementation and should be revalidated after evaluator/compiler changes.
- Eligible axes emit schema-v2 `{n, low, high}` edges. The change is shared by both nominal and uncertainty exporters. Current payloads were updated by replacing only certified edge representations, preserving contents, formulas, names and source constants. The runtime bundle manifest was updated to match compressed payload hashes. Original payloads remain in the campaign directory and Git history.
- Native CorrectionLib tests compare exact bin-index outputs for 100,010 edge, neighboring-value, random and overflow points for each of clamp and default-value flow, plus error-flow boundary checks. The eligibility guard rejects the known unsafe axis and nonuniform/infinite axes. NaN inputs are outside this finite-input equivalence claim.
- Existing six backend tests passed. All three updated C-nice runs passed the full A comparisons. Saved inputs, nominal outputs, all 117 kinematic states and 119 JVT/weight states match checkpoint 26 byte for byte, excluding changed provenance metadata.
- B passed the full 20,000-event A comparison with unchanged configuration; its single validation run took **265.05 s**. Loaded-library records confirm the expected overlays and CorrectionLib library. This is a single integration timing, not a repeated B speed estimate.
- No C++ implementation, public C-nice interface, B configuration, seed or precision setting changed.

Implementation commit: `0eee0f84dd4efc37f3211e487b14e408f1c861ff`. [Shared exporter](python/atlas_correctionlib/nodes.py), [native validation/conversion](uniform-binning/validate_and_convert.py), [conversion manifest](optimization-campaign/29-uniform-binning/conversion.json), [output parity](optimization-campaign/29-uniform-binning/parity.json), [acceptance](optimization-campaign/29-uniform-binning/acceptance.json).

A broader speed improvement would require a separate change to uniform indexing or payload structure with explicit boundary validation. Simply classifying approximately equally spaced edges as uniform would change some bin decisions.

All changes and evidence are committed locally in the authoritative WSL and Windows inspection repositories. Nothing was pushed.
