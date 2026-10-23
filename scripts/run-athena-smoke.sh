#!/usr/bin/env bash
cd /home/gwatts/atlas/correctionlib_tests
source scripts/setup-athena.sh
source build-athena/x86_64-el9-gcc15-opt/setup.sh
export DATAPATH="$ATLAS_CORRECTIONLIB_BUNDLE:${DATAPATH:-}"
set -e
python config/athena-smoke.py > logs/athena-smoke.log 2>&1
