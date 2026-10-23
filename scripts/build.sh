#!/usr/bin/env bash
cd /home/gwatts/atlas/correctionlib_tests
source setup.sh
set -e
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -Dcorrectionlib_DIR="$(python -c 'import correctionlib,pathlib;print(pathlib.Path(correctionlib.__file__).parent/"cmake")')" -Dpybind11_DIR="$(python -m pybind11 --cmakedir)" -DPython_EXECUTABLE="$(command -v python)" -DCMAKE_INSTALL_PREFIX="$PWD/install"
cmake --build build -j 4
cmake --install build
