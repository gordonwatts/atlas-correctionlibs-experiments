#!/usr/bin/env bash
set -euo pipefail

if test "$#" -ne 4; then
  echo "Usage: $0 ARCHIVE ROOT_FILE NEW_RESULT_DIR UV_EXECUTABLE" >&2
  exit 2
fi

archive=$(realpath -e "$1")
input=$(realpath -s "$2")
output=$(realpath -m "$3")
uv=$(realpath -e "$4")
if ! test -f "$input"; then
  echo "Input ROOT file does not exist: $input" >&2
  exit 2
fi
if test -e "$output"; then
  echo "Refusing to overwrite: $output" >&2
  exit 2
fi

mkdir -p "$output/work" "$output/results"
tar -xzf "$archive" -C "$output/work"
histogram="$(basename "$input").jet-pt-by-systematic.npz"
printf 'run,elapsed_seconds\n' > "$output/times.csv"
cd "$output/work"

for run in 1 2 3; do
  start_ns=$(date +%s%N)
  env -i HOME="$HOME" PATH=/usr/bin:/bin \
    UV_PYTHON_INSTALL_DIR="$output/python" \
    UV_CACHE_DIR="$output/cache" UV_NO_PROGRESS=1 \
    "$uv" run --managed-python run_complete_corrections.py "$input" \
    > "$output/results/run-$run.log" 2>&1
  stop_ns=$(date +%s%N)
  seconds=$(awk -v ns="$((stop_ns - start_ns))" 'BEGIN { printf "%.3f", ns / 1000000000 }')
  mv -- "$histogram" "$output/results/run-$run.npz"
  printf '%d,%s\n' "$run" "$seconds" | tee -a "$output/times.csv"
done
