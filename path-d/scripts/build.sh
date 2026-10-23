#!/usr/bin/env bash
cd /home/gwatts/atlas/correctionlib_tests
source setup.sh
set -e
python path-d/scripts/prepare-fork.py
cmake -S path-d -B path-d/build -DCMAKE_BUILD_TYPE=Release -Dpybind11_DIR="$(python -m pybind11 --cmakedir)" -DPython_EXECUTABLE="$(command -v python)"
cmake --build path-d/build -j4
