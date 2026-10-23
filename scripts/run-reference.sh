#!/usr/bin/env bash
set +e
cd /home/gwatts/atlas/correctionlib_tests
source setup.sh
setup_status=$?
if [ "$setup_status" -ne 0 ]; then exit "$setup_status"; fi
set -e
mkdir -p results/reference logs
input=$(cat /cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt/src/AsgExternal/Asg_Test/cmt/TestFileLiteR3MC.txt)
cd results/reference
CPRun.py -i "$input" -t ../../config/reference.yaml -o reference.root -e "${1:-100}" > ../../logs/reference.log 2>&1
