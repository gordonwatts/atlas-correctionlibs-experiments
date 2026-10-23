# Copyright (C) 2002-2020 CERN for the benefit of the ATLAS collaboration

# The name of the package:
atlas_subdir( JetJvtEfficiency )

# External(s):
find_package( ROOT COMPONENTS Core Hist RIO )

# Libraries of the package:
atlas_add_library( JetJvtEfficiencyLib
   JetJvtEfficiency/*.h Root/*.cxx
   PUBLIC_HEADERS JetJvtEfficiency
   INCLUDE_DIRS ${ROOT_INCLUDE_DIRS}
   LINK_LIBRARIES ${ROOT_LIBRARIES} AsgTools xAODJet xAODEventInfo JetInterface JetAnalysisInterfacesLib PATInterfaces JetMomentToolsLib PATCoreLib
   PRIVATE_LINK_LIBRARIES PathResolver )

atlas_add_component( JetJvtEfficiency
   src/components/*.cxx
   LINK_LIBRARIES JetJvtEfficiencyLib )

atlas_add_dictionary( JetJvtEfficiencyDict
   JetJvtEfficiency/JetJvtEfficiencyDict.h
   JetJvtEfficiency/selection.xml
   LINK_LIBRARIES JetJvtEfficiencyLib )

# Install files from the package:
atlas_install_python_modules( python/*.py )

# CorrectionLib demo overlay, generated from the pinned release.
find_package(correctionlib CONFIG REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(lwtnn REQUIRED)
target_include_directories(JetJvtEfficiencyLib PRIVATE "/home/gwatts/atlas/correctionlib_tests/src" ${LWTNN_INCLUDE_DIRS})
target_link_libraries(JetJvtEfficiencyLib PRIVATE correctionlib Eigen3::Eigen ${LWTNN_LIBRARIES})
