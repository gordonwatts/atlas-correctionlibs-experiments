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

The full timing harness creates isolated `results/C-N` folders under `c-nice`, uses this facade for nominal calibration, and then runs the unchanged C systematic and JVT diagnostic stages. Existing timing directories cause failure rather than overwrite. This full workflow is broader than the nominal-only facade. Historical A/B timing numbers and new C-nice numbers are not an interleaved controlled performance experiment.
