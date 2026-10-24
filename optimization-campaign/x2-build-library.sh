#!/bin/bash
cd /home/gwatts/atlas/correctionlib_tests
source setup.sh >/dev/null 2>&1
export PYTHONPATH="$PWD/c-nice:$PWD/install/python:$PWD/python:$PYTHONPATH"
set -e
mkdir -p optimization-campaign/x2-checks
cmake -S external/correctionlib -B build-correctionlib-x2 -DCMAKE_BUILD_TYPE=Release -DBUILD_DEMO=OFF -DSKBUILD_PROJECT_VERSION=2.7.1 > optimization-campaign/x2-checks/library-configure.log 2>&1
cmake --build build-correctionlib-x2 --target correctionlib -j 4 > optimization-campaign/x2-checks/library-build.log 2>&1
cp build-correctionlib-x2/libcorrectionlib.so venv/lib/python3.13/site-packages/correctionlib/lib/libcorrectionlib.so
python -m pytest -q tests > optimization-campaign/x2-checks/library-tests.log 2>&1
cat optimization-campaign/x2-checks/library-tests.log
