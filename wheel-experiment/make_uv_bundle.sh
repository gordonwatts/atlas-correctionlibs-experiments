#!/usr/bin/env bash
# Create a transferable bundle: PEP 723 runner plus the complete local wheel.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
wheel=$(realpath -e "$1")
output=$(realpath -m "$2")
if test -e "$output"; then
  echo "Refusing to overwrite: $output" >&2
  exit 2
fi
mkdir -p "$output/bundle"
cp "$root/wheel-experiment/run_complete_corrections.py" "$output/bundle/"
cp "$root/wheel-experiment/reference-fixture.npz" "$output/bundle/"
cp "$wheel" "$output/bundle/"
(cd "$output/bundle" && sha256sum run_complete_corrections.py reference-fixture.npz "$(basename "$wheel")" > SHA256SUMS)
tar -C "$output/bundle" -czf "$output/atlas-complete-corrections-uv314.tar.gz" .
stat -c '%n: %s bytes' "$output/atlas-complete-corrections-uv314.tar.gz"
