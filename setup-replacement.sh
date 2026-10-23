#!/usr/bin/env bash
source /home/gwatts/atlas/correctionlib_tests/setup.sh
source /home/gwatts/atlas/correctionlib_tests/build-overlay/x86_64-el9-gcc15-opt/setup.sh
export ATLAS_CORRECTIONLIB_BUNDLE=/home/gwatts/atlas/correctionlib_tests/payloads
export DATAPATH="$ATLAS_CORRECTIONLIB_BUNDLE:${DATAPATH:-}"
