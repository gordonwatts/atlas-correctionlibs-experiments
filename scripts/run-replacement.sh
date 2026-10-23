#!/usr/bin/env bash
cd /home/gwatts/atlas/correctionlib_tests
source setup-replacement.sh
set -e
mkdir -p results/replacement logs
input=$(cat /cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt/src/AsgExternal/Asg_Test/cmt/TestFileLiteR3MC.txt)
cd results/replacement
CPRun.py -i "$input" -t ../../config/reference.yaml -o replacement.root -e "${1:-100}" > ../../logs/replacement.log 2>&1
