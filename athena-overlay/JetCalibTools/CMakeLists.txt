# Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration

# Declare the package name:
atlas_subdir( JetCalibTools )

# External dependencies:
find_package( Boost )
find_package( ROOT COMPONENTS Core Tree MathCore Hist RIO Graf Gpad )
find_package( lwtnn )
find_package( onnxruntime )

# Libraries in the package:
atlas_add_library( JetCalibToolsLib
   JetCalibTools/*.h Root/*.cxx src/*.cxx
   PUBLIC_HEADERS JetCalibTools
   INCLUDE_DIRS ${Boost_INCLUDE_DIRS} ${ROOT_INCLUDE_DIRS} ${LWTNN_INCLUDE_DIRS} ${ONNXRUNTIME_INCLUDE_DIRS}
   LINK_LIBRARIES ${Boost_LIBRARIES} ${ROOT_LIBRARIES} ${LWTNN_LIBRARIES} ${ONNXRUNTIME_LIBRARIES} AsgTools AsgServicesLib xAODEventInfo xAODEventShape xAODMetaData
   xAODJet xAODTracking PATInterfaces JetInterface AsgDataHandlesLib AthOnnxInterfaces AsgServicesLib JetToolHelpersLib MuonAnalysisInterfacesLib JetAnalysisInterfacesLib
   PRIVATE_LINK_LIBRARIES xAODMuon PathResolver AthContainers FourMomUtils)

atlas_add_component( JetCalibTools
   src/components/*.cxx
   INCLUDE_DIRS ${ONNXRUNTIME_INCLUDE_DIRS}
   LINK_LIBRARIES ${ONNXRUNTIME_LIBRARIES} JetCalibToolsLib )

atlas_add_dictionary( JetCalibToolsDict
   JetCalibTools/JetCalibToolsDict.h
   JetCalibTools/selection.xml
   LINK_LIBRARIES JetCalibToolsLib )

# Executable(s) in the package:
set( _extraLibs )
if( NOT XAOD_STANDALONE )
   set( _extraLibs "POOLRootAccessLib" )
endif()

atlas_add_executable( JetCalibTools_Example
   util/JetCalibTools_Example.cxx
   INCLUDE_DIRS ${ROOT_INCLUDE_DIRS}
   LINK_LIBRARIES ${ROOT_LIBRARIES} AsgTools AsgServicesLib xAODEventInfo xAODJet
   xAODEventShape xAODCore xAODRootAccess JetCalibToolsLib ${_extraLibs} )
   
atlas_add_executable( JetCalibTools_PlotJESFactors
   util/JetCalibTools_PlotJESFactors.cxx
   INCLUDE_DIRS ${ROOT_INCLUDE_DIRS}
   LINK_LIBRARIES ${ROOT_LIBRARIES} AsgTools AsgServicesLib xAODEventInfo xAODJet
   xAODCore xAODRootAccess JetCalibToolsLib ${_extraLibs} )
   
atlas_add_executable( JetCalibTools_PlotJMSFactors
   util/JetCalibTools_PlotJMSFactors.cxx
   INCLUDE_DIRS ${ROOT_INCLUDE_DIRS}
   LINK_LIBRARIES ${ROOT_LIBRARIES} AsgTools AsgServicesLib xAODEventInfo xAODJet
   xAODCore xAODRootAccess JetCalibToolsLib CxxUtils ${_extraLibs} )

atlas_add_executable( JetCalibTools_SmearingPlots
   util/JetCalibTools_SmearingPlots.cxx
   INCLUDE_DIRS ${ROOT_INCLUDE_DIRS}
   LINK_LIBRARIES ${ROOT_LIBRARIES} AsgTools AsgServicesLib xAODEventInfo xAODJet
   xAODCore xAODRootAccess JetCalibToolsLib ${_extraLibs} )
   
atlas_install_python_modules( python/*.py )

atlas_install_data( data/* )
# CorrectionLib demo overlay, generated from the pinned release.
find_package(correctionlib CONFIG REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(lwtnn REQUIRED)
target_include_directories(JetCalibToolsLib PRIVATE "/home/gwatts/atlas/correctionlib_tests/src" ${LWTNN_INCLUDE_DIRS})
target_link_libraries(JetCalibToolsLib PRIVATE correctionlib Eigen3::Eigen ${LWTNN_LIBRARIES})
