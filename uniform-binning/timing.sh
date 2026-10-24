#!/usr/bin/env bash
cd /home/gwatts/atlas/correctionlib_tests
source setup.sh >/dev/null 2>&1
set -eo pipefail
export PYTHONPATH="$PWD/c-nice:$PWD/install/python:$PWD/python:${PYTHONPATH:-}"
export ATLAS_CORRECTIONLIB_THREADS=1 OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 MKL_NUM_THREADS=1
python uniform-binning/timing.py "$1"
