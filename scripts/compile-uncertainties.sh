#!/usr/bin/env bash
cd /home/gwatts/atlas/correctionlib_tests
source setup.sh
export PYTHONPATH="$PWD/python:${PYTHONPATH:-}"
python -m atlas_correctionlib.compile_uncertainties payloads
