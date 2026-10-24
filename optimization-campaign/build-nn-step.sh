#!/bin/bash
cd /home/gwatts/atlas/correctionlib_tests
source setup.sh >/dev/null 2>&1
export PYTHONPATH="$PWD/c-nice:$PWD/install/python:$PWD/python:$PYTHONPATH"
export ATLAS_CORRECTIONLIB_THREADS=1 OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1
set -e
mkdir -p optimization-campaign/nn-checks
cmake --build build -j 4 > "optimization-campaign/nn-checks/$1-build.log" 2>&1
cmake --install build >> "optimization-campaign/nn-checks/$1-build.log" 2>&1
cmake -S atlas-overlay -B build-overlay >> "optimization-campaign/nn-checks/$1-build.log" 2>&1
cmake --build build-overlay -j 3 >> "optimization-campaign/nn-checks/$1-build.log" 2>&1
python -m pytest -q tests > "optimization-campaign/nn-checks/$1-tests.log" 2>&1
cat "optimization-campaign/nn-checks/$1-tests.log"
