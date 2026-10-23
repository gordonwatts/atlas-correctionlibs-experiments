#!/usr/bin/env bash
cd /home/gwatts/atlas/correctionlib_tests
source scripts/setup-athena.sh
set -e
python3 scripts/prepare-athena.py
cmake -S athena-overlay -B build-athena -DCMAKE_BUILD_TYPE=RelWithDebInfo -Dcorrectionlib_DIR="$PWD/venv/lib/python3.13/site-packages/correctionlib/cmake"
cmake --build build-athena -j 3
