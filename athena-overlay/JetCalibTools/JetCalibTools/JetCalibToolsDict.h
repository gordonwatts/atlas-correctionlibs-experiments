/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#ifndef JETCALIBTOOLS_JETCALIBTOOLSDICT_H
#define JETCALIBTOOLS_JETCALIBTOOLSDICT_H 1

// Reflex dictionary generation
// Following instructions on the CP tools twiki:
// https://twiki.cern.ch/twiki/bin/viewauth/AtlasComputing/DevelopingCPToolsForxAOD
// Special handling for Eigen vectorization

#if defined(__GCCXML__) and not defined(EIGEN_DONT_VECTORIZE)
#define EIGEN_DONT_VECTORIZE
#endif

#include "JetCalibTools/JetCalibrationTool.h"
#include "JetCalibTools/JetCalibTool.h"
#include "JetCalibTools/MuonInJetCorrectionTool.h"
#include "JetCalibTools/BJetCorrectionTool.h"
#include "JetCalibTools/Pileup1DResidualCalibStep.h"
#include "JetCalibTools/PileupAreaResidualCalibStep.h"
#include "JetCalibTools/EtaJESCalibStep.h"
#include "JetCalibTools/SmearingCalibStep.h"
#include "JetCalibTools/GSCCalibStep.h"
#include "JetCalibTools/JMSCalibStep.h"
#include "JetCalibTools/InSituCalibStep.h"
#include "JetCalibTools/InSituJMSCalibStep.h"
#include "JetCalibTools/Generic4VecCorrectionStep.h"

#endif
