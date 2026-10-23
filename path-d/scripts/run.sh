#!/usr/bin/env bash
cd /home/gwatts/atlas/correctionlib_tests
source path-d/setup.sh
set -e
bash path-d/scripts/build.sh
python path-d/python/compile_graphs.py
python path-d/tests/test-architecture.py
python path-d/scripts/check-reproducibility.py
python path-d/tests/test-primitives.py
python path-d/tests/test-table-edges.py
python path-d/scripts/run-nominal.py --events 20000 --chunk-size 1000
python path-d/scripts/run-systematics.py --events 20000 --output path-d/results/kinematics.root
python path-d/scripts/run-jvt.py --events 20000 --output path-d/results/jvt.root
python path-d/tests/check-jes-boundaries.py
python path-d/tests/check-jvt-boundaries.py
python path-d/tests/check-truth-boundaries.py
python path-d/tests/check-uncertainty-boundaries.py
python path-d/tests/test-pipelines.py
python path-d/tests/test-determinism.py
python path-d/scripts/runtime-provenance.py
python path-d/scripts/report.py
