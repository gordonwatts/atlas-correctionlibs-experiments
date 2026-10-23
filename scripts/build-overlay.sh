#!/usr/bin/env bash
cd /home/gwatts/atlas/correctionlib_tests
source setup.sh
set -e
python scripts/prepare-overlay.py
cmake -S atlas-overlay -B build-overlay -DCMAKE_BUILD_TYPE=RelWithDebInfo -Dcorrectionlib_DIR="$(python -c 'import correctionlib,pathlib;print(pathlib.Path(correctionlib.__file__).parent/"cmake")')"
cmake --build build-overlay -j 3
