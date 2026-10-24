#!/bin/bash
cd /home/gwatts/atlas/correctionlib_tests
source setup.sh >/dev/null 2>&1
export PYTHONPATH="$PWD/c-nice:$PWD/install/python:$PWD/python:$PYTHONPATH"
export ATLAS_CORRECTIONLIB_THREADS=1 OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1
set -e
python /mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib/x2-parity.py "$1" "$2"
if [ "$2" = "13-bin-edge-reference" ]; then
 python -m pytest -q external/correctionlib/tests/test_core.py external/correctionlib/tests/test_core_valid.py > optimization-campaign/x2-checks/bin-core-tests.log 2>&1
 cat optimization-campaign/x2-checks/bin-core-tests.log
fi
