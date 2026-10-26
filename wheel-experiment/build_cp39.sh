#!/usr/bin/env bash
# Rebuild for AlmaLinux 9 x86_64 and the host CPython 3.9 interpreter.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
output=$(realpath -m "$1")
if test -e "$output"; then
  echo "Refusing to overwrite: $output" >&2
  exit 2
fi
mkdir -p "$output/python-devel-rpm" "$output/headers"
dnf download python3-devel --destdir "$output/python-devel-rpm" >"$output/python-devel-download.log" 2>&1
rpms=("$output"/python-devel-rpm/python3-devel-*.x86_64.rpm)
if test ! -f "${rpms[0]}"; then
  echo "python3-devel RPM was not downloaded" >&2
  exit 1
fi
(cd "$output/headers" && rpm2cpio "${rpms[0]}" | cpio -idmu --quiet)
set +eu
source "$root/setup.sh" >"$output/release-setup.log" 2>&1
setup_status=$?
set -eu
if test "$setup_status" -ne 0; then
  echo "ATLAS build setup failed; see $output/release-setup.log" >&2
  exit "$setup_status"
fi
correctionlib_dir=$(python -c 'import correctionlib,pathlib; print(pathlib.Path(correctionlib.__file__).parent / "cmake")')
pybind11_dir=$(python -m pybind11 --cmakedir)
cmake -S "$root" -B "$output/build" -DCMAKE_BUILD_TYPE=Release \
  -Dcorrectionlib_DIR="$correctionlib_dir" -Dpybind11_DIR="$pybind11_dir" \
  -DPYTHON_EXECUTABLE=/usr/bin/python3.9 \
  -DPYTHON_LIBRARY=/usr/lib64/libpython3.9.so.1.0 \
  -DCMAKE_CXX_FLAGS="-I$output/headers/usr/include/python3.9" \
  -DCMAKE_INSTALL_PREFIX="$output/install" >"$output/configure.log" 2>&1
cmake --build "$output/build" -j 4 >"$output/native-build.log" 2>&1
cmake --install "$output/build" >"$output/native-install.log" 2>&1
python "$root/wheel-experiment/build_wheel.py" \
  --native "$output/install/python/_atlas_corrections.cpython-39-x86_64-linux-gnu.so" \
  --python-tag cp39 --output "$output/wheel"
