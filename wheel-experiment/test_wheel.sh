#!/usr/bin/env bash
set -euo pipefail
# Run from a plain shell: no setupATLAS, asetup, or source of the project venv.
wheel=$1
fixture=$2
output=$3
bootstrap_python=$4
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
if test -e "$output"; then
  echo "Refusing to overwrite: $output" >&2
  exit 2
fi
mkdir -p "$output"
cd "$output"
env -i HOME="$HOME" PATH=/usr/bin:/bin "$bootstrap_python" -m venv --copies venv
env -i HOME="$HOME" PATH=/usr/bin:/bin venv/bin/python -m pip install "$wheel" >pip-install.log 2>&1
env -i HOME="$HOME" PATH=/usr/bin:/bin venv/bin/python -m pip check >pip-check.log 2>&1
env -i HOME="$HOME" PATH=/usr/bin:/bin venv/bin/python "$script_dir/check_installed.py" \
  --fixture "$fixture" --report "$output/acceptance.json" >check.log 2>&1
cat acceptance.json
