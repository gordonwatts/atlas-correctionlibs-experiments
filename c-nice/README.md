# C-nice

**C-nice facade timings include nominal recalibration.** Both the original and array-processing versions read `JetConstitScaleMomentum_*` from PHYSLITE and rerun pileup, JES and GSC calibration using the shared B/C backend. `NOSYS` requests nominal calibration without variations; it does not skip calibration. The 8.05-second plotting result includes that work. The separate `plot-stored-jets.py` baseline now reads stored pT/eta and plots without importing any calibration code: 1.08 ± 0.05 seconds, or 2.22 ± 0.02 seconds when also reading C-nice's calibration inputs. See `stored-timing/summary.json`. Applying uncertainties directly to stored momentum through the facade remains future work. Future measurements should separately specify nominal recalibration versus stored-momentum reuse, and systematics on versus off.

Nominal calibration facade around the unchanged Path B/C `_atlas_corrections.Nominal` backend. Authoritative implementation: `/home/gwatts/atlas/correctionlib_tests/c-nice` in WSL `atlas_al9`. Windows files are inspection copies.

```bash
cd /home/gwatts/atlas/correctionlib_tests
source setup.sh
export PYTHONPATH="$PWD/c-nice:$PWD/install/python:$PYTHONPATH"
MPLBACKEND=Agg python c-nice/plot-jets.py
```

The plotting script contains the frozen sample and bundle paths. It writes `jet-pt.png` in the current directory. `JetCorrections.load` accepts the local directory containing `nominal-manifest.json` and `nominal.json.gz`. Input units are original PHYSLITE units, including MeV for momenta. `evaluate` supports only `systematics=["NOSYS"]`, `outputs=["calibration"]`; unsupported requests fail explicitly.

`result["NOSYS"]` contains event-by-jet Awkward arrays `pt`, `eta`, `phi`, `m`, `valid`. Validity means finite intermediate/final coordinates; evaluator exceptions propagate. `result["stages"]`, `result["inputs"]` and `result["counts"]` support the diagnostic workflow. `result["metadata"]` records the resolved bundle path and systematic choice. Bundle paths are not content hashes; preserve the frozen payloads when reproducing results.

The facade preserves the original scalar input arithmetic and float32 rounding. Collection lengths and required fields are checked before calling C++. Empty chunks and zero-jet events return correctly shaped results. Jets in an event with no vertices raise an error. A nonempty collection without a primary vertex uses index zero, matching Path C. Numerical singularities and backend exceptions propagate rather than silently replacing corrections.

Input preparation now uses Awkward/NumPy array operations throughout: no Python iteration over events or jets. Iteration over fixed field names and input chunks remains. The existing C++ batch conversion/evaluation is unchanged. Full-sample prepared inputs and outputs match the previous version bitwise. `array-validation.json` records the check. Remeasured timings for this update are in `array-timing/summary.json`; the original `summary.json` retains the earlier implementation timings. The full diagnostic workflow and the nominal-only plot have different output/validation scope; a future controlled campaign should compare with and without systematics under matched conditions.

The full timing harness creates isolated `results/C-N` folders under `c-nice`, uses this facade for nominal calibration, and then runs the C systematic and JVT diagnostic stages. Existing timing directories cause failure rather than overwrite. This full workflow is broader than the nominal-only facade. Historical A/B timing numbers and new C-nice numbers are not an interleaved controlled performance experiment.

The subsequent full-systematics optimization campaign is recorded in [the checkpoint table](../c-nice-optimization.md), with individual implementation commits and reference checks. Those measurements supersede the historical full-workflow timing above; the nominal plotting and no-calibration measurements have not been rerun in this campaign. Dense-array bindings now transfer batches in native code, while both B and C still use the shared scalar physics implementation. The uncertainty backend compiles component metadata once. The systematic and JVT runners use bounded native concurrency, array-based JVT preparation, LZ4 output, and lazy Python metadata loading.

`ATLAS_CORRECTIONLIB_THREADS` controls native batch concurrency (default one; set a larger value only for an explicit parallel experiment). It changes neither B's configuration nor the facade call. Threaded and sequential outputs were checked bitwise. More threads are not guaranteed to improve total runtime.

The JVT runner accepts `--kinematics results/columnar-kinematics.root` to reuse the preceding systematic output. It validates input and payload/backend fingerprints, state names, event IDs, jet ordering and statuses before reuse. Omitting this option retains the recomputation path. This cache is local to the diagnostic workflow and is not a new facade requirement. Array-based truth matching constructs per-event jet/truth pairs and can consume more temporary memory in high-multiplicity samples.

The earlier full-systematics single-core timing was **49.63 ± 0.39 s**. See [the RNG and profiling follow-up](../single-thread-profiling.md). The shared smearing implementation retains ROOT's exact random stream while avoiding redundant seeding and reusing first Gaussian draws with width-dependent rejection preserved.


The subsequent exact-result single-core campaign selected **26.44 ± 0.42 s** (1.88× versus 49.63 s; 1.78× versus its rebuilt control). See [the per-change table and acceptance report](../x2-optimization.md). All A comparisons and outputs remain enabled, final C arrays match the prior checkpoint byte for byte, and B passed with unchanged configuration. The 5,000-event chunk experiment was reverted; 1,000-event chunks remain selected.
