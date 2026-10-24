#!/bin/bash
cd /home/gwatts/atlas/correctionlib_tests
source setup.sh >/dev/null 2>&1
export PYTHONPATH="$PWD/c-nice:$PWD/install/python:$PWD/python:$PYTHONPATH"
export ATLAS_CORRECTIONLIB_THREADS=1
python /mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib/build-check-smearing.py "$1"
