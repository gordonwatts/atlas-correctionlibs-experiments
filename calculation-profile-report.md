# Higher-resolution calculation profile

NNJVT is the largest remaining measured calculation cost. Much of its native work is allocating and freeing tiny intermediate vectors, and many variations repeat exactly the same NN features. Both findings suggest optimizations that can be tested without lowering numerical precision.

## Full-sample wall timers

These timers surround the actual native batch calls in the selected checkpoint-20 workflow, on one CPU, with 20,000 events and all A comparisons enabled. All comparisons passed. This is a diagnostic run, not a new repeated performance benchmark.

| Calculation | Calls | Seconds |
|---|---:|---:|
| NNJVT native batches | 111 | 3.145 |
| JVT efficiency native batches | 111 | 0.595 |
| Grouped systematic native batches | 45 | 1.594 |
| Complete JVT evaluate calls, including the two rows above | 119 | 4.290 |
| Exact-cache equality checks, included in JVT total | 148 | 0.068 |

Eight of the 119 JVT calls avoided native evaluation through the existing whole-batch cache. The remaining approximately 0.55 seconds in the JVT total includes array construction, output conversion, cache operations and profiling sample capture; it is not a clean measurement of production Python overhead. Native-call timers stop before capture. Systematic batches produce all 117 kinematic states.

The earlier measured reference-read cost (7.53 s), tool-loading cost (3.75 s) and ROOT-writing cost (2.55 s) remain separate concerns. This study focuses on calculation internals; it does not further subdivide startup or file I/O.

## What makes NNJVT expensive

The payload is a small **4 → 8 → 5 → 3 → 1** dense network: **90 weights**, with ReLU hidden activations and a final sigmoid. It does not require a large matrix calculation per jet.

The native instruction profile replays 2,048 evenly spaced jets from each of the 111 batches that actually reached NN evaluation. Of 227,328 sampled rows, 117,430 invoked the network; the others followed the existing validity/selection paths.

| NNJVT operation | Share of sampled instructions |
|---|---:|
| `malloc`, `free`, `_int_free`, exclusive counts summed | **41.23%** |
| Eigen matrix–vector multiply kernel, exclusive counts summed | **14.46%** |
| Entire graph evaluation, inclusive | **76.29%** |

The graph row includes much of the allocation and arithmetic work, so these rows must not be added. Instruction shares are not CPU-time shares.

[The NN wrapper](src/nnjvt.hxx) constructs a feature vector and Eigen input vectors for each active jet. In the installed lwtnn implementation, `Stack::compute` repeatedly assigns the vector returned by each layer. Matrix, bias and activation operations are separate layers, and the graph also returns vectors by value. The profile attributes substantial allocation/free work to these paths. Reusing the outer NumPy adapter's buffer, as already implemented, does not remove these internal NN allocations.

## Exact repeated inference

For each sampled jet, I reconstructed the four inputs passed to the NN: float32-rounded Rpt, JVFCorr, pt-bin and eta-bin. Equality was tested on their bit representations, only for rows where the network runs.

| Feature reuse in sampled active inferences | Count |
|---|---:|
| Active network evaluations | 117,430 |
| Distinct feature tuples, counted separately for each jet | 76,611 |
| Repeated feature tuples within a jet | 40,819 (34.8%) |
| Later evaluations matching that jet's first/nominal batch | 40,749 (34.7%) |
| Later evaluations matching the immediately preceding batch | 25,082 (21.4%) |

A per-jet nominal-feature cache could therefore avoid about a third of the **sampled active inferences**, beyond the whole-batch cache already in place. This is not a predicted one-third reduction in total NN time or workflow time: comparisons, selection and other processing still cost time. The sample fraction may change with the dataset.

Such a cache would reuse only the NN score when its effective inputs match exactly. Selection cuts, validity and efficiency weights must still be evaluated for the current variation. It would also need stable jet identity, tool/payload invalidation and appropriate lifetime handling. It has not been implemented or benchmarked in this study.

## Efficiency and systematic calculations

Efficiency evaluation takes only **0.595 s** for the full workflow. In its native sample, table-wrapper evaluation accounts for **53.37% inclusive** of instructions, and the three allocator functions above account for **33.84% exclusive**. The table path creates temporary argument vectors for CorrectionLib, and the result also uses a small vector. [The efficiency source](src/efficiency.hxx) additionally reads error tables when sigma is zero, then multiplies the errors by zero. Avoiding those lookups is a possible small change, but would require explicit checks of validity, non-finite values and exact numerical behavior before acceptance.

Grouped systematics take **1.594 s**. The table wrapper accounts for **72.68% inclusive** of its sampled instructions; CorrectionLib's nested binning, formula evaluation and transforms dominate that path. Allocator functions account for **11.27% exclusive**. The sampled replay begins with a cold Gaussian cache and only 2,048 distinct input jets, so its RNG shares should not be extrapolated to the full production traversal. This study does not establish a new case for replacing the RNG.

## Recommended experiment order

1. **Exact per-jet NN feature reuse.** A relatively narrow experiment supported by measured duplicate inputs. Preserve selection and weight evaluation, then check byte-level output parity and full A/B acceptance.
2. **Reusable NN working buffers.** Address the large allocation cost inside lwtnn. Preserve operation ordering and activations where possible; fused or fixed-size kernels can change floating-point rounding and require separate numerical validation.
3. **Remove small efficiency argument/result allocations and unnecessary zero-sigma lookups.** Smaller absolute opportunity because the entire efficiency calculation is below 0.6 seconds here.
4. **Specialize repeated systematic table access.** The remaining kernel is table-driven, but its total time is already about 1.6 seconds. Any specialization should preserve edge, overflow and interpolation behavior.

No measured end-to-end speedup is claimed for these proposals. No production physics code, B configuration or C-nice interface changed in this study.

## Method and evidence

Native profiles use Callgrind instruction counts, with constructors and input-file reading outside the instrumented regions. The replay uses the current shared headers and installed libraries, compiled with `-O2 -g`; it is a kernel study rather than a cycle profile of the installed Python extension. Inputs come from actual evaluated batches after cache decisions. Native profiles exclude Python/NumPy conversion overhead. The systematic replay uses the actual grouped interface and all configured groups.

Callgrind inclusive attribution can exceed 100% for recursive graph/formula functions. Conclusions about allocation use exclusive counts to avoid double-counting. Valgrind emitted a `brk segment overflow` limitation warning, but all replay programs completed; allocator instruction counts are evidence for investigation, not a direct estimate of allocator wall time on the production run. Instrumented elapsed times are not used as native performance measurements.

The raw native traces, binary input samples and replay executable remain in WSL `optimization-campaign/24-calculation-profile/`. Lightweight evidence and scripts are committed in both repositories:

- [Full-sample timers](optimization-campaign/24-calculation-profile/detailed-timers.json)
- [Feature reuse and instruction-count analysis](optimization-campaign/24-calculation-profile/analysis.json)
- [NN exclusive profile](optimization-campaign/24-calculation-profile/nn-no.txt) and [inclusive profile](optimization-campaign/24-calculation-profile/nn-yes.txt)
- [Efficiency exclusive profile](optimization-campaign/24-calculation-profile/efficiency-no.txt)
- [Systematics inclusive profile](optimization-campaign/24-calculation-profile/systematics-yes.txt)
- [Build/sample metadata](optimization-campaign/24-calculation-profile/metadata.json)

All work is committed locally; nothing was pushed.
