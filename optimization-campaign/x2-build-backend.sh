#!/bin/bash
cd /home/gwatts/atlas/correctionlib_tests
source setup.sh >/dev/null 2>&1
export PYTHONPATH="$PWD/c-nice:$PWD/install/python:$PWD/python:$PYTHONPATH"
export ATLAS_CORRECTIONLIB_THREADS=1
set -e
cmake --build build -j 4 > "optimization-campaign/x2-checks/$1-build.log" 2>&1
cmake --install build >> "optimization-campaign/x2-checks/$1-build.log" 2>&1
cmake --build build-overlay -j 3 >> "optimization-campaign/x2-checks/$1-build.log" 2>&1
python -m pytest -q tests > "optimization-campaign/x2-checks/$1-tests.log" 2>&1
cat "optimization-campaign/x2-checks/$1-tests.log"
if [ "$1" = "15-paired-systematics" ]; then
 python /mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib/x2-check-groups.py > optimization-campaign/x2-checks/group-tests.log
 cat optimization-campaign/x2-checks/group-tests.log
fi
