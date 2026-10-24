#!/bin/bash
cd /home/gwatts/atlas/correctionlib_tests
source setup-replacement.sh >/dev/null 2>&1
export ATLAS_CORRECTIONLIB_THREADS=1 OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 MKL_NUM_THREADS=1
python /mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib/x2-check-b.py

