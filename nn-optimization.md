# Shared NN feature caching and reusable buffers

**Selected full-systematics result: 25.61 ± 0.31 s on one CPU.** Both NN changes are implemented in the shared B/C inference backend. This is 1.94× faster than the 49.63 s checkpoint and 0.83 s below the pre-NN 26.44 s checkpoint. These are short sequential measurements, not a controlled estimate of each change’s isolated gain.

| Checkpoint | Full C-nice seconds | WSL implementation commit |
|---|---:|---|
| Before these NN changes (20) | 26.44 ± 0.42 | `38935839d523edfad166fcb740fdcb79deb8ed88` |
| Exact feature cache (25) | 27.01 ± 1.00 | `e35bd9d9a171aee5eeecf68016f44aea058b0e1f` |
| Cache plus reusable buffers (26) | 25.61 ± 0.31 | `2d8cad68da093634555819df49ea7190e4285023` |

Three runs per checkpoint; discard the first and report mean/sample SD of the other two. Same 20,000 events, 196,606 jets, nominal recalibration, all 117 kinematic and 119 JVT/weight states, outputs and A comparisons. Input chunks remain 1,000 events, with one-CPU affinity and one native worker. The cache alone did not demonstrate a full-workflow gain. Buffer reuse reduced the measured mean, but the small sample and variation limit precise attribution. No precision or acceptance-tolerance changes were made.

## Implementation

[Shared inference wrapper](src/nn_graph.hxx): exact feature memoization uses all four double input bit patterns, after the existing caller’s float rounding. A model-specific identity prevents reuse across payloads or object lifetimes. Entries are thread-local and bounded to 262,144 slots, approximately 12 MiB per thread. Each slot retains its first tuple, favoring nominal features; a collision evaluates normally. Non-finite inputs bypass the cache. This is feature-keyed rather than keyed by jet position, so it works for B’s scalar traversal and C’s array batches without depending on chunk size, ordering or jet identity. Equal features from another jet are safe to reuse because inference depends only on these features and the immutable model. Selection, validity and weights are still evaluated for the current variation.

[Reusable buffers](src/nn_buffered.hxx): a conservative execution plan handles the supported single-input dense chain. It reuses thread-local Eigen input/intermediate vectors, preserves double arithmetic, and keeps matrix multiplication, bias, ReLU and sigmoid operations separate. Input reordering, normalization and sigmoid clipping follow lwtnn. Unsupported graph forms continue through the original FastGraph implementation. No graph rewrite, reduced precision or activation approximation is used.

C’s [NN evaluator](src/nnjvt.hxx) calls this backend. B previously used the stock ATLAS NN tagger for inference; its [tagger overlay](atlas-overlay/JetMomentTools/Root/JetVertexNNTagger.cxx) now uses the same backend. The overlay is based on the pinned AnalysisBase 25.2.100 JetMomentTools package. Configuration, feature preparation and jet-selection logic remain unchanged. The C-nice public interface is unchanged.

## Acceptance

- Each checkpoint passed every full-sample A comparison in all three timing runs.
- Saved inputs, nominal stages and every kinematic/JVT output buffer match checkpoint 20 byte for byte. Checkpoint 26 also matches checkpoint 25. Dtypes, IDs and statuses are included; intentionally changed provenance metadata is excluded.
- 302,104 exact score comparisons passed against the original lwtnn calculation, covering repeats, cache collisions, reverse traversal, thread isolation, model switching and extreme/signed-zero inputs.
- Buffer checks confirmed the optimized path was active, performed no Eigen heap allocations after warmup, preserved reordered input semantics and fell back correctly for unsupported activation. A staging-header include in an early test was corrected; acceptance uses the authoritative WSL header and the stronger active-path test.
- The existing six backend tests passed after each native build.
- B passed the complete 20,000-event comparison with A. Its one-core integration run took **268.07 s**; this is one validation run, not a repeated B performance estimate. Process mappings confirm both the new `libJetMomentToolsLib.so` and the other shared overlay libraries were loaded. Configuration SHA-256 remains `4d84aa237b6a1796c05e9a38fa341e89993f1b8c582bc99da7b1624b4ed46ca8`.

The [complete timing history](x2-optimization.md) now includes the original optimization baseline, all intermediate checkpoints, the four-thread interval, reverted experiments, these NN steps, earlier A/B/C/D comparisons, nominal plotting and no-calibration baselines. These scopes are labeled separately.

Evidence: [B summary](optimization-campaign/nn-B-integration/summary.json), [loaded libraries](optimization-campaign/nn-B-integration/loaded-libraries.txt), [buffer tests](optimization-campaign/nn-checks/check-26-buffer-edges.log), and [full output parity](optimization-campaign/x2-checks/20-flat-adapters-to-26-nn-reusable-buffers-parity.json).

All changes, checks and documentation are committed locally in both repositories. Nothing was pushed.
