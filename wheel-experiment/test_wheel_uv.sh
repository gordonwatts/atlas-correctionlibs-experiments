#!/usr/bin/env bash
# Install and validate the CPython 3.14 wheel without the ATLAS release setup.
set -euo pipefail
wheel=$(realpath -e "$1")
fixture=$(realpath -e "$2")
output=$(realpath -m "$3")
uv_bin=${4:-$(command -v uv)}
uv_bin=$(realpath -e "$uv_bin")
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
if test -e "$output"; then
  echo "Refusing to overwrite: $output" >&2
  exit 2
fi
mkdir -p "$output"
cd "$output"

# Every uv invocation uses this campaign's Python and cache directories.
clean=(env -i HOME="$HOME" PATH=/usr/bin:/bin \
  UV_PYTHON_INSTALL_DIR="$output/python" UV_PYTHON_BIN_DIR="$output/bin" \
  UV_CACHE_DIR="$output/cache" UV_NO_PROGRESS=1)
"${clean[@]}" "$uv_bin" --version >uv-version.txt
"${clean[@]}" "$uv_bin" venv --python 3.14 --managed-python "$output/venv" >venv.log 2>&1
"${clean[@]}" "$uv_bin" pip install --python "$output/venv/bin/python" "$wheel" >uv-install.log 2>&1
"${clean[@]}" "$uv_bin" pip check --python "$output/venv/bin/python" >uv-check.log 2>&1
"${clean[@]}" "$output/venv/bin/python" -c \
  'import sys; assert sys.version_info[:2] == (3, 14), sys.version' >python-version-check.log 2>&1
"${clean[@]}" "$output/venv/bin/python" "$script_dir/check_installed.py" \
  --fixture "$fixture" --report "$output/acceptance.json" >check.log 2>&1
cat acceptance.json
