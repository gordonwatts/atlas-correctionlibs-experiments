#!/usr/bin/env bash
cd /home/gwatts/atlas/correctionlib_tests
source setup.sh >/dev/null 2>&1
set -eo pipefail
export OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 ATLAS_CORRECTIONLIB_THREADS=1
python lookup-benchmark/prepare.py
optimization-campaign/28-lookup-comparison-v2/benchmark "$PWD" "$PWD/optimization-campaign/28-lookup-comparison-v2" > optimization-campaign/28-lookup-comparison-v2/run.log 2>&1

