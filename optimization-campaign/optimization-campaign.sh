#!/bin/bash
cd /home/gwatts/atlas/correctionlib_tests
source setup.sh >/dev/null 2>&1
export PYTHONPATH=/home/gwatts/atlas/correctionlib_tests/c-nice:/home/gwatts/atlas/correctionlib_tests/install/python:$PYTHONPATH
python /mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib/optimization-campaign.py "$1"
