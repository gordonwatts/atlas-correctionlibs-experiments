#!/bin/bash
cd /home/gwatts/atlas/correctionlib_tests
source setup.sh >/dev/null 2>&1
export PYTHONPATH=/home/gwatts/atlas/correctionlib_tests/c-nice:/home/gwatts/atlas/correctionlib_tests/install/python:$PYTHONPATH
python c-nice/timing.py C 1 > c-nice/run-1.log 2>&1 || exit 1
python c-nice/timing.py C 2 > c-nice/run-2.log 2>&1 || exit 1
python c-nice/timing.py C 3 > c-nice/run-3.log 2>&1 || exit 1
python c-nice/timing.py C 4 > c-nice/run-4.log 2>&1 || exit 1
