#!/usr/bin/env bash
cd /home/gwatts/atlas/correctionlib_tests
source setup-replacement.sh >/dev/null 2>&1
set -eo pipefail
export ATLAS_CORRECTIONLIB_THREADS=1 OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 MKL_NUM_THREADS=1
mkdir optimization-campaign/27-final-B-timing
git rev-parse HEAD > optimization-campaign/27-final-B-timing/source-commit.txt
for run in 1 2 3 4; do
    echo "Starting B run ${run}/4 (first excluded from mean)"
    python final-b-benchmark/run-one.py "run-${run}"
done
python - <<'PY'
from pathlib import Path
import json, statistics
p = Path('optimization-campaign/27-final-B-timing')
runs = [json.loads((p/f'run-{i}/summary.json').read_text()) for i in range(1,5)]
times = [r['wall_seconds'] for r in runs[1:]]
report = dict(runs=runs, excluded_runs=[1], mean_seconds=statistics.mean(times), sample_sd_seconds=statistics.stdev(times), historical_B_mean_seconds=290.70, historical_B_sd_seconds=1.01, historical_comparison_not_interleaved=True)
(p/'summary.json').write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps(report), flush=True)
PY
