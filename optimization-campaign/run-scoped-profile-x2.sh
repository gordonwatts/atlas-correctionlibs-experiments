#!/bin/bash
cd /home/gwatts/atlas/correctionlib_tests
source setup.sh >/dev/null 2>&1
export PYTHONPATH="$PWD/c-nice:$PWD/install/python:$PWD/python:$PYTHONPATH"
export ATLAS_CORRECTIONLIB_THREADS=1 OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 MKL_NUM_THREADS=1
python /mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib/run-scoped-profile-x2.py "$1"
