#!/usr/bin/env bash
export ATLAS_LOCAL_ROOT_BASE=/cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase
source "$ATLAS_LOCAL_ROOT_BASE/user/atlasLocalSetup.sh" --quiet
asetup AnalysisBase,25.2.100
export CORRECTIONLIB_TESTS=/home/gwatts/atlas/correctionlib_tests
if test -d "$CORRECTIONLIB_TESTS/venv"; then source "$CORRECTIONLIB_TESTS/venv/bin/activate"; fi

if test -d "$CORRECTIONLIB_TESTS/venv/lib/python3.13/site-packages/correctionlib/lib"; then
  export LD_LIBRARY_PATH="$CORRECTIONLIB_TESTS/venv/lib/python3.13/site-packages/correctionlib/lib:${LD_LIBRARY_PATH:-}"
fi
