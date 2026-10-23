# ATLAS jet corrections with CorrectionLib

This is a working, pinned demonstration in the atlas_al9 WSL2 distro. All builds and execution stay under /home/gwatts/atlas/correctionlib_tests.

Three paths are compared:

- A: untouched AnalysisBase 25.2.100.
- B: rebuilt ATLAS calibration, uncertainty and JVT-efficiency packages, using the same analysis YAML and public tool/component names.
- C: original PHYSLITE columns read with Uproot/Awkward and evaluated through the shared C++ backend.

The numerical acceptance sample contains 20,000 events and 196,606 jets. There are 117 nominal/JES/JER states, including pseudo-data JER, plus two NNJVT efficiency variations. The reduced model is Category JES / Full JER, not All/All. The acceptance report is [results/validation-summary.md](results/validation-summary.md); the complete machine-readable result is [results/validation-summary.json](results/validation-summary.json).

## Start here

Read implementation-plan.md and docs/calculation-inventory.md for the calculation, constants, ROOT-object conversion, interpolation, composition, precision and configuration details.

The main files are:

| File or directory | Purpose |
|---|---|
| config/reference.yaml | Identical A/B analysis configuration |
| config/reference.frozen.yaml | Frozen configuration copy |
| config/environment.json | Tested compiler, ROOT, Python, releases and packages |
| config/requirements-wheels.lock | Exact private Python dependency versions |
| python/atlas_correctionlib/compile_nominal.py | Nominal payload compiler |
| python/atlas_correctionlib/compile_uncertainties.py | Uncertainty and JVT table compiler |
| payloads/*manifest.json | Source/object traceability, settings and checksums |
| payloads/nominal.json.gz | Ten corrections and the five-stage GSC compound |
| payloads/uncertainty-tables.json.gz | Exported uncertainty/SF/efficiency tables |
| src/backend.hxx, systematics.hxx, efficiency.hxx, nnjvt.hxx | Shared numerical layer |
| scripts/prepare-overlay.py | Reproducible ATLAS adapter source generation |
| setup-replacement.sh | Selects B without modifying analysis YAML |
| scripts/run-columnar-*.py | Original-input columnar runners and comparisons |
| results/reference/reference.root | Authoritative A output |
| results/replacement/replacement.root | B output |
| results/columnar-nominal.npz | C intermediate calibration states |
| results/columnar-kinematics.root | C per-systematic momenta, identities and statuses |
| results/columnar-jvt.root | C scores, decisions, jet/event weights and statuses |

ROOT columnar files contain one object per systematic. Records retain event number and original jet index; input SHA-256 and entry identity are recorded in the run manifest. Status values in saved columnar files use CP::CorrectionCode conventions: Error=0, OutOfValidityRange=1, Ok=2. Internal shared C++ interfaces use documented zero-success status values and adapters translate them.

## Build and run

Run these commands inside atlas_al9:

```bash
cd ~/atlas/correctionlib_tests
bash scripts/bootstrap.sh
bash scripts/compile-payloads.sh
bash scripts/compile-uncertainties.sh
python3 scripts/export-nnjvt.py
python3 scripts/package-bundle.py
bash scripts/build.sh
bash scripts/build-overlay.sh

bash scripts/run-reference.sh 20000
bash scripts/run-replacement.sh 20000

source setup.sh
python scripts/compare-atlas.py
python scripts/run-columnar-nominal.py --events 20000 --chunk-size 1000
python scripts/run-columnar-systematics.py --events 20000
python scripts/run-columnar-jvt.py --events 20000
```

scripts/run-demo.sh executes the complete build, validation, PHYS/Athena smoke-test and report workflow. It intentionally replaces generated outputs in this workspace.

The reference and replacement run in separate processes and environment setups. Never combine competing ATLAS components in one process. The adapters accept the pinned configuration and reject unsupported calibration, uncertainty or JVT configurations. This is a validated demonstration for one recommendation bundle, not a general replacement for every ATLAS jet configuration or every specialist tool-query API.

## Additional validation

```bash
bash scripts/test.sh
source setup.sh
python scripts/check-jes-boundaries.py
python scripts/check-jvt-boundaries.py
python scripts/test-determinism.py
python scripts/test-systematic-determinism.py

bash scripts/run-phys-smoke.sh reference
bash scripts/run-phys-smoke.sh replacement
python scripts/compare-atlas.py --reference results/phys-reference/phys.root --candidate results/phys-replacement/phys.root --report results/phys-comparison.json

bash scripts/build-athena.sh
bash scripts/run-athena-smoke.sh
bash scripts/check-bundle-only.sh
```

The Athena smoke test pins Athena 25.0.76 and builds the adapters against that framework. It initializes five tools with zero events; numerical authority remains AnalysisBase. The PHYS test uses its own source collection and standard preprocessing. It compares A against B on PHYS, not PHYS against PHYSLITE.

The accepted tolerances are fixed in the implementation plan. Identifiers, counts, coverage, decisions and statuses are checked exactly where recorded. Synthetic original-tool probes cover JES extrapolation/freezing/eta boundaries and JVT weights/statuses at validity boundaries. Unit tests cover ROOT interpolation, JSON precision, negative pile-up reset, special GSC inputs and TRandom3 seeding/sign/rejection behavior. Determinism reports check the complete sample with shuffled inputs, different chunks and one/four workers.

## CorrectionLib precision patch and build details

CorrectionLib is based on v2.7.0 at fd69c2ef33edbe9a1d4bc714430d6ccda374be00. A required local patch enables full-precision RapidJSON parsing. Without it, one-bit changes to loaded constants caused near-zero masses to fail the fixed tolerance. With it, nominal intermediate four-vectors match exactly on the full sample. See patches/correctionlib-full-precision-json.patch and docs/correctionlib-upstream-note.md.

No custom schema node was required. CorrectionLib provides scalar composition and histogram interpolation formulas; C++ handles four-vectors, float rounding, ATLAS-compatible random draws and neural-network execution. Original calibration ROOT files are compiler inputs, not replacement runtime dependencies. The bundle-only smoke test restricts DATAPATH to the packaged payload directory.

The ATLAS Python in this environment has an OpenSSL compatibility issue. Bootstrap uses the distro's system Python for verified HTTPS wheel downloads, then installs those locked wheels offline into the ATLAS Python environment. It builds CorrectionLib's C++ library and Python bindings from the same patched source. TLS verification is not disabled.

## Timing

results/performance.json separates initialization, direct original-column reads, cached-feature loading and numerical evaluation, with peak resident memory. It is a 1,000-event component benchmark with possibly warm filesystem caches, not an end-to-end throughput claim. Feature reconstruction time is explicitly outside that benchmark. Logs retain the AnalysisBase job's own timing and memory measurements.

## Scope

Supported here: small-R EMPFlow MC23a nominal calibration, Category JES, Full JER/pseudo-data, classic NNJVT FixedEffPt selection, and jet/event efficiency weights. Excluded: large-R jets, forward JVT, flavour-tagging weights and MET propagation. Additional recommendation bundles require explicit conversion, property mapping and independent validation before use.

## Implementation history and repeated timings

- [Code-reviewed implementation log](docs/implementation-log.md) explains the build sequence and current limitations.
- [Repeated full-sample timing report](results/repeated-timing/summary.md) reports four runs per numerical path, excluding the first from each average.
- [Review source hashes](docs/implementation-review.json) identify the code inspected for the log.

## Fourth path: declarative D

The isolated [path-D implementation](path-d/README.md) moves correction physics, precision points, branches and per-jet stage order into payload graphs, with an optional generic CorrectionLib graph extension. Its [acceptance report](path-d/results/validation-summary.md) covers all 20,000 events and all 119 output states; [design notes](path-d/docs/design.md) explain the reusable primitives and remaining boundaries. A/B/C remain unchanged. Path D has its own build, payloads, outputs and [workflow timing](path-d/results/timing.md).
