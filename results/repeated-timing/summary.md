# Repeated full-sample timings

All runs: 20,000 events, 196,606 jets, atlas_al9 WSL2; AnalysisBase 25.2.100 toolchain. Seconds of wall-clock time.

| Path | Run 1 (excluded) | Run 2 | Run 3 | Run 4 | Mean (2–4) | Sample SD (2–4) |
|---|---:|---:|---:|---:|---:|---:|
| A — untouched AnalysisBase | 242.03 | 237.90 | 236.16 | 237.72 | 237.26 | 0.96 |
| B — CorrectionLib ATLAS tools | 294.24 | 291.07 | 289.55 | 291.48 | 290.70 | 1.01 |
| C — columnar validation workflow | 168.91 | 167.85 | 168.57 | 166.68 | 167.70 | 0.95 |

Wall time after environment setup; imports, initialization, input preparation, computation and output included. C also includes reference comparisons and recalculates kinematics in its JVT stage. A/B comparisons are outside timed jobs. These measure existing workflows, not isolated evaluator speed.

No cache flushing; first run discarded per path; CVMFS/OS caches may already be warm.

Each run has its own outputs and logs under results/repeated-timing. All runs are checked against untouched AnalysisBase. See summary.json for commands, individual phase times, and unrounded measurements.

Athena was not included: its previous test only initialized tools with zero events.
