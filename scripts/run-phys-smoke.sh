#!/usr/bin/env bash
cd /home/gwatts/atlas/correctionlib_tests
correctionlib_phys_mode="${1:-reference}"
if [ "$correctionlib_phys_mode" = replacement ]; then source setup-replacement.sh; elif [ "$correctionlib_phys_mode" = reference ]; then source setup.sh; else exit 2; fi
set -e
mkdir -p "results/phys-$correctionlib_phys_mode"
input=$(cat /cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt/src/AsgExternal/Asg_Test/cmt/TestFileR3MC.txt)
cd "results/phys-$correctionlib_phys_mode"
CPRun.py -i "$input" -t ../../config/reference.yaml -o phys.root -e 100 > "../../logs/phys-$correctionlib_phys_mode.log" 2>&1
