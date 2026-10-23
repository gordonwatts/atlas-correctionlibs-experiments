#!/bin/bash
cd /home/gwatts/atlas/correctionlib_tests
source setup.sh >/dev/null 2>&1
python -m pip install --no-index --find-links c-nice/wheels hist mplhep==0.4.1 || exit 1
export PYTHONPATH=/home/gwatts/atlas/correctionlib_tests/c-nice:/home/gwatts/atlas/correctionlib_tests/install/python:$PYTHONPATH
export MPLBACKEND=Agg
cd c-nice
python plot-jets.py
