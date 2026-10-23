#!/usr/bin/env bash
export ATLAS_LOCAL_ROOT_BASE=/cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase
source "$ATLAS_LOCAL_ROOT_BASE/user/atlasLocalSetup.sh" --quiet
asetup Athena,25.0.76
export CORRECTIONLIB_TESTS=/home/gwatts/atlas/correctionlib_tests
export ATLAS_CORRECTIONLIB_BUNDLE="$CORRECTIONLIB_TESTS/payloads"
export LD_LIBRARY_PATH="$CORRECTIONLIB_TESTS/venv/lib/python3.13/site-packages/correctionlib/lib:${LD_LIBRARY_PATH:-}"
