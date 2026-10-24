// this file is -*- C++ -*-

/*
  Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration
*/

#ifndef JETMOMENTTOOLS_JETCALOENERGIES_H
#define JETMOMENTTOOLS_JETCALOENERGIES_H

#include "AsgTools/AsgTool.h"
#include "AsgTools/PropertyWrapper.h"
#include "JetInterface/IJetDecorator.h"
#include "AsgDataHandles/WriteDecorHandleKey.h"
#include "xAODJet/JetContainer.h"
#include "xAODCaloEvent/CaloCluster.h"
#include <vector>

class JetCaloEnergies : public asg::AsgTool,
                        virtual public IJetDecorator {
  ASG_TOOL_CLASS0(JetCaloEnergies)
public:
  
  JetCaloEnergies(const std::string & t);

  virtual StatusCode initialize() override;
  virtual StatusCode decorate(const xAOD::JetContainer& jets) const override;

protected:
  void fillEperSamplingCluster(const xAOD::Jet &jet, std::vector<float> & ePerSampling ) const ;
  void fillEperSamplingPFO(const xAOD::Jet &jet, std::vector<float> & ePerSampling ) const ;
  void fillEperSamplingFE(const xAOD::Jet &jet, std::vector<float> & ePerSampling ) const ;
  void fillEperSamplingFEClusterBased(const xAOD::Jet &jet, std::vector<float> & ePerSampling ) const ;
  bool isInVector(const std::string& key, const std::vector<std::string>& calculations);
  bool m_doFracSamplingMax = false;

private:
  Gaudi::Property<std::vector<std::string> > m_calculationNames{this, "Calculations", {}, "Name of calo quantities to compute and add as decorations"};
  Gaudi::Property<std::string> m_jetContainerName{this, "JetContainer", "", "SG key for the input jet container"};
  Gaudi::Property<bool> m_calcClusterBasedVars{this, "calcClusterBasedVars", false, "SG key to decide if cluster-based variables will be calculated for FE-based jets"};

  SG::WriteDecorHandleKey<xAOD::JetContainer> m_ePerSamplingKey{this, "EPerSamplingName", "EnergyPerSampling", "SG key for the EnergyPerSampling attribute"};
  SG::WriteDecorHandleKey<xAOD::JetContainer> m_emFracKey{this, "EMFracName", "EMFrac", "SG key for the EMFrac attribute"};
  SG::WriteDecorHandleKey<xAOD::JetContainer> m_hecFracKey{this, "HECFracName", "HECFrac", "SG key for the HECFrac attribute"};
  SG::WriteDecorHandleKey<xAOD::JetContainer> m_psFracKey{this, "PSFracName", "PSFrac", "SG key for the PSFrac attribute"};
  SG::WriteDecorHandleKey<xAOD::JetContainer> m_em3FracKey{this, "EM3FracName", "EM3Frac", "SG key for the EM3Frac attribute"};
  SG::WriteDecorHandleKey<xAOD::JetContainer> m_tile0FracKey{this, "Tile0FracName", "Tile0Frac", "SG key for the Tile0Frac attribute"};
  SG::WriteDecorHandleKey<xAOD::JetContainer> m_effNClustsFracKey{this, "EffNClustsName", "EffNClusts", "SG key for the EffNClusts attribute"};
  SG::WriteDecorHandleKey<xAOD::JetContainer> m_fracSamplingMaxKey{this, "FracSamplingMaxName", "FracSamplingMax", "SG key for the FracSamplingMax attribute"};
  SG::WriteDecorHandleKey<xAOD::JetContainer> m_fracSamplingMaxIndexKey{this, "FracSamplingMaxIndexName", "FracSamplingMaxIndex", "SG key for the FracSamplingMaxIndex attribute"};

  // Variables for FE-based jet collection using the underlying cluster rather than energy-subtracted FE for calculations
  SG::WriteDecorHandleKey<xAOD::JetContainer> m_ePerSamplingClusterKey{this, "EPerSamplingClusterName", "EnergyPerSamplingCaloBased", "SG key for the EnergyPerSampling attribute"};
  SG::WriteDecorHandleKey<xAOD::JetContainer> m_emFracClusterKey{this, "EMFracClusterName", "EMFracCaloBased", "SG key for the EMFrac attribute"};
  SG::WriteDecorHandleKey<xAOD::JetContainer> m_hecFracClusterKey{this, "HECFracClusterName", "HECFracCaloBased", "SG key for the HECFrac attribute"};
  SG::WriteDecorHandleKey<xAOD::JetContainer> m_psFracClusterKey{this, "PSFracClusterName", "PSFracCaloBased", "SG key for the PSFrac attribute"};
  SG::WriteDecorHandleKey<xAOD::JetContainer> m_em3FracClusterKey{this, "EM3FracClusterName", "EM3FracCaloBased", "SG key for the EM3Frac attribute"};
  SG::WriteDecorHandleKey<xAOD::JetContainer> m_tile0FracClusterKey{this, "Tile0FracClusterName", "Tile0FracCaloBased", "SG key for the Tile0Frac attribute"};
  SG::WriteDecorHandleKey<xAOD::JetContainer> m_effNClustsFracClusterKey{this, "EffNClustsClusterName", "EffNClustsCaloBased", "SG key for the EffNClusts attribute"};
  SG::WriteDecorHandleKey<xAOD::JetContainer> m_fracSamplingMaxClusterKey{this, "FracSamplingMaxClusterName", "FracSamplingMaxCaloBased", "SG key for the FracSamplingMax (clus) attribute"};
  SG::WriteDecorHandleKey<xAOD::JetContainer> m_fracSamplingMaxIndexClusterKey{this, "FracSamplingMaxIndexClusterName", "FracSamplingMaxIndexCaloBased", "SG key for FracSamplingMaxIndex (clus) attribute"};
  
  SG::WriteDecorHandleKey<xAOD::JetContainer> m_lambdaLeadingClusterKey{this, "LambdaLeadingClusterName", "LambdaLeadingCluster", "SG key for the longitudinal distance between the leading (highest energy) cluster and the jet’s geometrical center"};
  SG::WriteDecorHandleKey<xAOD::JetContainer> m_meanRadialDistanceSquaredKey{this, "MeanRadialDistanceSquaredName", "MeanRadialDistanceSquared", "SG key for the weighted mean of the squared radial distances of the clusters from jet’s geometrical center"};
  SG::WriteDecorHandleKey<xAOD::JetContainer> m_meanLongitudinalDistanceSquaredKey{this, "MeanLongitudinalDistanceSquaredName", "MeanLongitudinalDistanceSquared", "SG key for the weighted mean of the squared Longitudinal distances of the clusters from jet’s geometrical center"};

private:
  float getMoment(const xAOD::CaloCluster* cluster, const xAOD::CaloCluster::MomentType& momentType) const;

};


#undef ASG_DERIVED_TOOL_CLASS
#endif

