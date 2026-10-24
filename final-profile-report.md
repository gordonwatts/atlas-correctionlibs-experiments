# Profile of the selected C-nice implementation

A fresh profile of the selected checkpoint-20 implementation confirms that reference-data reading is now the largest individually measured section. The direct elapsed-time run took **27.62 s** including process startup. This is one diagnostic run, not a replacement for the **26.44 ± 0.42 s** benchmark. It used the same 20,000 events, 1,000-event chunks, one-CPU affinity, one native worker, all outputs and all A comparisons. Both new profile runs passed those comparisons.

| Measured section | Elapsed seconds |
|---|---:|
| Read A references for systematics and JVT | 7.53 |
| JVT evaluation, including array preparation and cache | 4.42 |
| Load nominal, systematic and JVT tools | 3.75 |
| Write ROOT output objects | 2.55 |
| Produce all 117 systematic variations | 1.63 |
| Nominal input preparation and evaluation | 0.71 |
| Remaining time, not separately attributed | 7.03 |
| **Outer process total** | **27.62** |

These scoped timers surround separate operations. The remainder includes process/import overhead, PHYSLITE reading, nominal reference reads, saved-kinematics reading and checks, comparisons and array reductions outside the wrappers, NPZ output, and other orchestration. It has not been subdivided. Tool loading breaks down into nominal 2.30 s, systematics 0.68 s and JVT 0.76 s. The 7.53 s reference-read number includes only the grouped systematic/JVT reference reader, not every input read.

The existing uninstrumented benchmark stage means remain nominal 4.43 s, systematics 9.86 s and JVT 11.74 s. Those broader stage totals include I/O and validation and must not be added to the table above.

## What this suggests

The next substantial target is reference-data access: a more efficient reference representation or read strategy could accelerate this diagnostic benchmark while preserving every comparison. Simply omitting reference validation would change the benchmark scope and would not establish another optimization of the same workload. A production analysis normally would not compare every variation to a saved A output.

JVT is the largest measured calculation section. A native profile of NNJVT and efficiency evaluation is the next step to split its 4.42 s further. This report measures the complete JVT call, including array preparation and exact-cache checks; it does not claim that all of that time is neural-network inference. Tool construction and serialization also remain meaningful costs. No new physics approximation is justified by this report alone.

## Evidence and limits

The fresh cProfile run is retained for inspection, but some entries have inconsistent call accounting (for example, zero primitive calls and zero cumulative time despite nonzero self time). Its reported function-time total is about 40 s and includes substantial instrumentation effects. Do not treat its cumulative entries as an additive wall-time budget or scale them directly to the 26.44 s benchmark. The table above comes from independent perf_counter wrappers around the selected operations, not from those cProfile entries. No new C++ instruction profile was captured in this follow-up.

[Scoped measurements](optimization-campaign/23-final-scoped-profile/scoped-profile.json), [outer timing](optimization-campaign/23-final-scoped-profile/timing.json), [cProfile text](optimization-campaign/22-final-python-profile/python-stage-0.txt), and [all recorded Python functions](optimization-campaign/22-final-python-profile/functions.json). The raw pstats file remains in WSL under `optimization-campaign/22-final-python-profile/runs/C-1/profile-0.pstats`.

The instrumentation is confined to separate profiling scripts and directories; the selected physics implementation and benchmark configuration were not changed. Profiles, comparisons and this report are preserved with local commits. Nothing was pushed.
