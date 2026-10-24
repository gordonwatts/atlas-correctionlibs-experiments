#!/usr/bin/env bash
cd /home/gwatts/atlas/correctionlib_tests
source setup.sh >/dev/null 2>&1
set -eo pipefail
export PYTHONPATH="$PWD/c-nice:$PWD/install/python:$PWD/python:${PYTHONPATH:-}"
export ATLAS_CORRECTIONLIB_THREADS=1 OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 MKL_NUM_THREADS=1
python -m pytest -q tests > optimization-campaign/29-uniform-binning/backend-tests.log 2>&1
python uniform-binning/timing.py 29-uniform-binning > optimization-campaign/29-uniform-binning/timing.log 2>&1
python /mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib/x2-parity.py 26-nn-reusable-buffers 29-uniform-binning > optimization-campaign/29-uniform-binning/parity.log 2>&1
set +e
source setup-replacement.sh >/dev/null 2>&1
set -e
python uniform-binning/check-b.py > optimization-campaign/29-uniform-binning/B-integration.log 2>&1
