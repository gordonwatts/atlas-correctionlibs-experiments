#!/usr/bin/env bash
cd /home/gwatts/atlas/correctionlib_tests
source setup-replacement.sh
set -e
export DATAPATH="$ATLAS_CORRECTIONLIB_BUNDLE"
mkdir -p results/bundle-only
input=$(cat /cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt/src/AsgExternal/Asg_Test/cmt/TestFileLiteR3MC.txt)
cd results/bundle-only
if command -v strace >/dev/null; then
 strace -f -e trace=openat -o ../../logs/bundle-open-files.log CPRun.py -i "$input" -t ../../config/reference.yaml -o bundle.root -e 1 > ../../logs/bundle-only.log 2>&1
else
 CPRun.py -i "$input" -t ../../config/reference.yaml -o bundle.root -e 1 > ../../logs/bundle-only.log 2>&1
fi
