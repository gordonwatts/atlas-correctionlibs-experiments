/*
  Copyright (C) 2002-2021 CERN for the benefit of the ATLAS collaboration
*/

#ifndef JETCALIBUTILS_H
#define JETCALIBUTILS_H

#include "TFile.h"
#include "TTree.h"
#include "TString.h"
#include "TObjString.h"
#include "TH1.h"
#include "TH2.h"
#include "TH3.h"
#include <cmath>
#include <vector>
#include <memory>
#include "xAODTracking/Vertex.h"

#define CHECK_THEN_ERROR( checkcode, message) if( checkcode ) { ATH_MSG_ERROR( message ) ; return StatusCode::FAILURE;}

typedef std::vector<TString> StrV;
typedef std::vector<double> VecD;

namespace JetCalibUtils {

  // Vectorizing functions and associated typedefs
  StrV Vectorize (const TString& str, const TString& sep=" ");
  VecD VectorizeD(const TString& str, const TString& sep=" ");

  // Read histogram from file and transfer ownership
  std::unique_ptr<const TH1> GetHisto(TFile& file, const TString& hname);
  std::unique_ptr<const TH2> GetHisto2(TFile& file, const TString& hname);
  std::unique_ptr<const TH3> GetHisto3(TFile& file, const TString& hname);

  TTree *setTree(TTree *tree);

  template<typename VXCONT>
  int countNPV(const VXCONT& vxCont){
      int eventNPV = 0;  
      for(const xAOD::Vertex* vtx: vxCont){
	if ( vtx->vertexType() == xAOD::VxType::PriVtx || vtx->vertexType() == xAOD::VxType::PileUp) ++eventNPV;
      }
      return eventNPV;
  }

}

#endif
