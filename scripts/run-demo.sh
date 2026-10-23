#!/usr/bin/env bash
set -e
cd /home/gwatts/atlas/correctionlib_tests
bash scripts/bootstrap.sh
bash scripts/compile-payloads.sh
bash scripts/compile-uncertainties.sh
python3 scripts/export-nnjvt.py
python3 scripts/package-bundle.py
bash scripts/build.sh
bash scripts/build-overlay.sh
bash scripts/run-reference.sh 20000
bash scripts/run-replacement.sh 20000
set +e
source setup.sh
correctionlib_setup_status=$?
if [ "$correctionlib_setup_status" -ne 0 ]; then exit "$correctionlib_setup_status"; fi
set -e
python scripts/compare-atlas.py
python scripts/run-columnar-nominal.py --events 20000 --chunk-size 1000
python scripts/run-columnar-systematics.py --events 20000
python scripts/run-columnar-jvt.py --events 20000
python scripts/run-columnar-systematics.py --events 20000 --reference results/replacement/replacement.root --report results/bc-kinematics-comparison.json --output results/columnar-kinematics.root
python scripts/run-columnar-jvt.py --events 20000 --reference results/replacement/replacement.root --report results/bc-jvt-comparison.json --output results/columnar-jvt.root
bash scripts/test.sh > logs/tests.log 2>&1
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
python scripts/benchmark.py
python scripts/freeze-environment.py
python scripts/summarize-validation.py
