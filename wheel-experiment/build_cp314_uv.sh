#!/usr/bin/env bash
# Build a CPython 3.14 wheel with a uv-managed interpreter and isolated output.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
output=$(realpath -m "$1")
uv_bin=${2:-$(command -v uv)}
uv_bin=$(realpath -e "$uv_bin")
if test -e "$output"; then
  echo "Refusing to overwrite: $output" >&2
  exit 2
fi
mkdir -p "$output"

# Keep uv's interpreter, executables, and cache inside this campaign.
clean=(env -i HOME="$HOME" PATH=/usr/bin:/bin \
  UV_PYTHON_INSTALL_DIR="$output/python" UV_PYTHON_BIN_DIR="$output/bin" \
  UV_CACHE_DIR="$output/cache" UV_NO_PROGRESS=1)
"${clean[@]}" "$uv_bin" --version >"$output/uv-version.txt"
"${clean[@]}" "$uv_bin" python install 3.14 >"$output/python-install.log" 2>&1
managed_python=$("${clean[@]}" "$uv_bin" python find 3.14 --managed-python)
managed_include=$("$managed_python" -c 'import sysconfig; print(sysconfig.get_path("include"))')
managed_library=$("$managed_python" -c 'import pathlib,sysconfig; print(pathlib.Path(sysconfig.get_config_var("LIBDIR")) / sysconfig.get_config_var("LDLIBRARY"))')
"${clean[@]}" "$uv_bin" venv --python 3.14 --managed-python "$output/build-venv" >"$output/build-venv.log" 2>&1
"${clean[@]}" "$uv_bin" pip install --python "$output/build-venv/bin/python" setuptools wheel >"$output/build-deps.log" 2>&1

# The release supplies build-time ROOT, lwtnn, and correctionlib headers/libs.
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
cmake -S "$root" -B "$output/native-build" -DCMAKE_BUILD_TYPE=Release \
  -Dcorrectionlib_DIR="$correctionlib_dir" -Dpybind11_DIR="$pybind11_dir" \
  -DPYTHON_EXECUTABLE="$managed_python" -DPYTHON_LIBRARY="$managed_library" \
  -DCMAKE_CXX_FLAGS="-I$managed_include" -DCMAKE_INSTALL_PREFIX="$output/native-install" \
  >"$output/native-configure.log" 2>&1
cmake --build "$output/native-build" -j 4 >"$output/native-build.log" 2>&1
cmake --install "$output/native-build" >"$output/native-install.log" 2>&1
"$output/build-venv/bin/python" "$root/wheel-experiment/build_wheel.py" \
  --native "$output/native-install/python/_atlas_corrections.cpython-314-x86_64-linux-gnu.so" \
  --python-tag cp314 --output "$output/wheel"
