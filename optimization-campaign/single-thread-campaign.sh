#!/bin/bash
cd /home/gwatts/atlas/correctionlib_tests
source setup.sh >/dev/null 2>&1
export PYTHONPATH="$PWD/c-nice:$PWD/install/python:$PWD/python:$PYTHONPATH"
export ATLAS_CORRECTIONLIB_THREADS=1 OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 MKL_NUM_THREADS=1
python - "$1" <<'PY'
import os,runpy,sys
os.sched_setaffinity(0,{min(os.sched_getaffinity(0))})
sys.argv=['optimization-campaign.py',sys.argv[1]]
runpy.run_path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib/optimization-campaign.py',run_name='__main__')
PY
