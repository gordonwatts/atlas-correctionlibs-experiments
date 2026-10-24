/*
  Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration
*/

#include "JetMomentTools/JetCaloQualityToolFE.h"
#include "xAODJet/JetAccessorMap.h"
#include "AsgDataHandles/WriteDecorHandle.h"

#include "xAODCaloEvent/CaloCluster.h"
#include "xAODPFlow/FlowElement.h"
#include "PFlowUtils/FEHelpers.h"

#include <iostream>
#include <iomanip>
using namespace std;

JetCaloQualityToolFE::JetCaloQualityToolFE(const std::string& name)
  : asg::AsgTool(name)
{
} 

StatusCode JetCaloQualityToolFE::initialize() {
  ATH_MSG_DEBUG( "Inside initialize() method" );

  if(!m_writeDecorKeys.empty()){
    ATH_MSG_ERROR("OutputDecorKeys should not be configured manually!");
    return StatusCode::FAILURE;
  }
  if(m_jetContainerName.empty()){
    ATH_MSG_ERROR("JetCaloQualityToolFE needs to have its input jet container name configured!");
    return StatusCode::FAILURE;
  }

  // Set the DecorHandleKeys with the correct strings
  for( const std::string & calcN : m_calculationNames){

    if(calcN == "LArQuality"){
      m_doLArQ = true;
    }
    else if(calcN == "HECQuality"){
      m_doHECQ = true;
    }
    else if(calcN == "NegativeE"){
      m_doNegE = true;
    }
    else if(calcN == "AverageLArQF"){
      m_doAvgLAr = true;
    }
    else if(calcN == "Timing"){
      m_doTime = true;
    }
    else if(calcN == "Centroid"){
      m_doCentroid = true;
    }
    else if(calcN == "BchCorrCell"){
      m_doBchCorrCell = true;
    }

    if(calcN == "Centroid"){
      m_writeDecorKeys.emplace_back(m_jetContainerName + "." + calcN+"R");
    }
    else{
      m_writeDecorKeys.emplace_back(m_jetContainerName + "." + calcN);
    }
  }

  // Define OOT calculators.
  for( const double timeCut : m_timingTimeCuts){
    // build the moment name from the base-name and the value of the timing cut
    std::stringstream s;
    s << std::setprecision(0) << std::fixed << "OotFracClusters" << timeCut;
    m_writeDecorKeys_OOT.emplace_back(m_jetContainerName + "." + s.str());
  }

  // Define tresholds for NXConstituents:
  for( const int fracCut: m_thresholdCuts){
    std::ostringstream sout;
    sout << "N" << fracCut << "Constituents";
    m_writeDecorKeys_Nfrac.emplace_back(m_jetContainerName + "." + sout.str());
  }

  ATH_CHECK(m_writeDecorKeys.initialize());
  if(!m_writeDecorKeys_OOT.empty()){
    ATH_CHECK(m_writeDecorKeys_OOT.initialize());
  }
  if(!m_writeDecorKeys_Nfrac.empty()){
    ATH_CHECK(m_writeDecorKeys_Nfrac.initialize());
  }

  return StatusCode::SUCCESS;
}

StatusCode JetCaloQualityToolFE::decorate(const xAOD::JetContainer& jets) const
{

  ATH_MSG_VERBOSE("Begin decorating jets.");

  for(const xAOD::Jet* jet : jets) {
    fillQualityVariables(*jet);
  }

  return StatusCode::SUCCESS;
}

std::vector<const xAOD::CaloCluster*> JetCaloQualityToolFE::extractConstituents(const xAOD::Jet& jet) const{

  std::vector<const xAOD::CaloCluster*> clusters;

  // Get the input type:
  xAOD::Type::ObjectType ctype = jet.rawConstituent(0)->type();

  if ( ctype  == xAOD::Type::CaloCluster ) {
    ATH_MSG_VERBOSE("  Constituents are calo clusters.");
    for ( size_t i = 0; i < jet.numConstituents(); i++ ) { 
      const xAOD::CaloCluster* constit = static_cast<const xAOD::CaloCluster*>(jet.rawConstituent(i)); 
      clusters.push_back(constit);
    }
  }
  else if( ctype  == xAOD::Type::FlowElement ){

    ATH_MSG_VERBOSE("  Constituents are FlowElements.");

    //Need to distinguish between ParticleFlow and UFOs
    const xAOD::FlowElement* constit0 = static_cast<const xAOD::FlowElement*>(jet.rawConstituent(0));

    // If jet constituents are ParticleFlow objects (stored as FlowElements)
    if(constit0->signalType() & xAOD::FlowElement::PFlow){
      ATH_MSG_VERBOSE("  Constituents are ParticleFlow objects stored as FlowElements.");
      for ( size_t i = 0; i < jet.numConstituents(); i++ ) {
	const xAOD::FlowElement* constit = static_cast<const xAOD::FlowElement*>(jet.rawConstituent(i));
	// Use only neutral PFOs
	if(constit->isCharged())
	  continue;

	if(constit->nOtherObjects() >= 1){
	  const xAOD::CaloCluster* cluster = dynamic_cast<const xAOD::CaloCluster*>(constit->otherObject(0));
	  if(cluster != nullptr){
	    clusters.push_back(cluster);
	  }
	}
      }
    }	
    else{ // jet constituents are UFOs (stored as FlowElements)
      ATH_MSG_VERBOSE("  Constituents are UFOs stored as FlowElements.");

      for ( size_t i = 0; i < jet.numConstituents(); i++ ) {
	const xAOD::FlowElement* constit = static_cast<const xAOD::FlowElement*>(jet.rawConstituent(i));

	//Reject charged UFOs (but keep combined UFOs)
	if(constit->signalType()==xAOD::FlowElement::SignalType::Charged)
	  continue;

	// For UFOs, otherObjects are links to the underlying ParticleFlow objects
	for (size_t n = 0; n < constit->otherObjects().size(); ++n) {
	  if(! constit->otherObject(n)) continue;
	  int index_pfo = constit->otherObject(n)->index();
	  if(index_pfo<0) continue;

	  const auto* fe = (constit->otherObject(n));
	  const xAOD::CaloCluster* cluster = nullptr;
	  
	  if(fe->type() == xAOD::Type::FlowElement){
	    const xAOD::FlowElement* pfo = dynamic_cast<const xAOD::FlowElement*>(fe);
	    if(!pfo->otherObjects().empty() && pfo->otherObject(0) && pfo->otherObject(0)->type() == xAOD::Type::CaloCluster){
	      cluster = dynamic_cast<const xAOD::CaloCluster*> (pfo->otherObject(0));
	    }
	  }
	  if(!cluster){continue;}

	  if(std::find(clusters.begin(), clusters.end(), cluster) == clusters.end()){
	    clusters.push_back(cluster);
	  }
	}
      }
    }
  }

  return clusters;

}

void JetCaloQualityToolFE::fillQualityVariables(const xAOD::Jet& jet) const{

  // First, extract the constituents directly (in case of cluster-based jet) or underlying clusters
  std::vector<const xAOD::CaloCluster*> clusters = extractConstituents(jet);

  // Calculate moments   
  float sum_E = 0.0;
  float sum_E_square = 0.0;

  float sum_badLarQ = 0.0;
  float sum_badHECQ = 0.0;
  float sum_e_HEC = 0.0;
  float sum_e_neg = 0.0;
  float sum_avg_lar_q = 0.0;
  float sum_timing = 0.0;
  float centroid_x = 0.0, centroid_y = 0.0, centroid_z = 0.0;
  float sum_e_bad_cells = 0.0;

  std::vector<float> sum_OOT;
  sum_OOT.resize(m_timingTimeCuts.size());

  std::vector<int> counter_Nfrac;
  counter_Nfrac.resize(m_thresholdCuts.size());

  std::vector<float> cluster_energies;

  for ( size_t i = 0; i < clusters.size(); i++){

    const xAOD::CaloCluster* constit = static_cast<const xAOD::CaloCluster*>(clusters[i]);

    float cluster_E = constit->e(xAOD::CaloCluster::UNCALIBRATED);

    sum_E += cluster_E;
    sum_E_square += cluster_E*cluster_E;

    cluster_energies.push_back(cluster_E);

    //LArQuality || HECQuality
    double bad_frac=0.0;
    if(m_doLArQ || m_doHECQ){
      constit->retrieveMoment(xAOD::CaloCluster::BADLARQ_FRAC, bad_frac);
      
      if(m_doLArQ){
	 sum_badLarQ += bad_frac*cluster_E;
      }

      if(m_doHECQ){
	float e_HEC = constit->eSample( CaloSampling::HEC0) + constit->eSample( CaloSampling::HEC1) + constit->eSample( CaloSampling::HEC2) + constit->eSample( CaloSampling::HEC3);
	sum_e_HEC += e_HEC;
	sum_badHECQ += bad_frac*e_HEC;
      }
    }

    //NegativeE
    if(m_doNegE){
      double e_pos=0.0;
      constit->retrieveMoment(xAOD::CaloCluster::ENG_POS, e_pos);
      sum_e_neg += cluster_E - e_pos;
    }

    //JetCalcAverageLArQualityF
    if(m_doAvgLAr){
      double avg_lar_q=0.0;
      constit->retrieveMoment(xAOD::CaloCluster::AVG_LAR_Q, avg_lar_q);
      sum_avg_lar_q += avg_lar_q*cluster_E*cluster_E;
    }

    //Centroid
    if(m_doCentroid){
      double x = 0.0, y = 0.0, z = 0.0;
      constit->retrieveMoment(xAOD::CaloCluster::CENTER_X, x);
      constit->retrieveMoment(xAOD::CaloCluster::CENTER_Y, y);
      constit->retrieveMoment(xAOD::CaloCluster::CENTER_Z, z);

      centroid_x += x*cluster_E;
      centroid_y += y*cluster_E;
      centroid_z += z*cluster_E;
    }

    //BchCorrCell
    if(m_doBchCorrCell){
      double cells_bad_E = 0.0;
      constit->retrieveMoment(xAOD::CaloCluster::ENG_BAD_CELLS, cells_bad_E);
      sum_e_bad_cells += cells_bad_E;
    }

    //Timing / OOT
    if(m_doTime || m_timingTimeCuts.size() > 0){
      double timing = constit->time();
      
      if(m_doTime){
	sum_timing += timing*cluster_E*cluster_E;
      }

      //OOT
      for(size_t j = 0; j < m_timingTimeCuts.size(); j++){
	if(std::abs(timing) > m_timingTimeCuts[j]){
	  sum_OOT[j] += cluster_E;
	}
      }
    }
  } // end loop over all all constituents


  if(m_thresholdCuts.size() > 0){

    std::sort(cluster_energies.rbegin(),cluster_energies.rend());

    for(size_t iFracCut = 0; iFracCut < m_thresholdCuts.size(); iFracCut++){

      int counter = 0;
      float tmp_sum = 0;

      for(unsigned int iClus = 0; iClus < cluster_energies.size(); iClus++){
	tmp_sum += cluster_energies[iClus];
	counter++;
	if(tmp_sum > m_thresholdCuts[iFracCut]*sum_E/100.) break;
      }
      counter_Nfrac[iFracCut] = counter;
    }
  }
  
  //Add the decorations
  for(size_t i = 0; i < m_calculationNames.size(); i++){
    std::string calcN = m_calculationNames[i];

    SG::WriteDecorHandle<xAOD::JetContainer, float> decHandle(m_writeDecorKeys.at(i));

    if(calcN == "LArQuality"){
      decHandle(jet) = sum_E != 0. ? sum_badLarQ/sum_E : 0.;
    }
    else if(calcN == "HECQuality"){
      decHandle(jet) = sum_e_HEC != 0. ? sum_badHECQ/sum_e_HEC : 0.;
    }
    else if(calcN == "NegativeE"){
      decHandle(jet) = sum_e_neg;
    }
    else if(calcN == "AverageLArQF"){
      decHandle(jet) = sum_E_square != 0. ? sum_avg_lar_q/sum_E_square : 0.;
    }
    else if(calcN == "Timing"){
      decHandle(jet) = sum_E_square != 0. ? sum_timing/sum_E_square : 0.;
    }
    else if(calcN == "Centroid"){
      decHandle(jet) = sum_E_square != 0. ? sqrt(centroid_x*centroid_x+centroid_y*centroid_y+centroid_z*centroid_z)/sum_E_square : 0.;
    }
    else if(calcN == "BchCorrCell"){
      decHandle(jet) = jet.jetP4(xAOD::JetEMScaleMomentum).E() != 0. ? sum_e_bad_cells/jet.jetP4(xAOD::JetEMScaleMomentum).E() : 0.   ;
    }
  }

  for( size_t iCut = 0; iCut < m_timingTimeCuts.size(); iCut++){
    SG::WriteDecorHandle<xAOD::JetContainer, float> decHandle_timing(m_writeDecorKeys_OOT.at(iCut));
    decHandle_timing(jet) = sum_E != 0. ? sum_OOT[iCut]/sum_E : 0. ;
  }

  for( size_t iFracCut = 0; iFracCut < m_thresholdCuts.size(); iFracCut++){
    //Variable was previously stored as float rather than int
    //Keep float to not break e.g. jet calibration with derivations storing variable as float
    SG::WriteDecorHandle<xAOD::JetContainer, float> decHandle_frac(m_writeDecorKeys_Nfrac.at(iFracCut));
    decHandle_frac(jet) = counter_Nfrac[iFracCut];
  }
}
