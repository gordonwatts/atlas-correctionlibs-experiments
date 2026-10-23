/*
  Copyright (C) 2002-2023 CERN for the benefit of the ATLAS collaboration
*/

#ifndef JETCALIBTOOLS_RESIDUALOFFSETCORRECTION_H
#define JETCALIBTOOLS_RESIDUALOFFSETCORRECTION_H 1

/* Implementation of ResidualOffsetCorrection class
 * This class will apply the residual offset pile up correction
 *
 * Author: Joe Taenzer (joseph.taenzer@cern.ch)
 * Date: August 15 2013
 */


#include "AsgMessaging/AsgMessaging.h"
#include "AsgMessaging/StatusCode.h"
#include "TString.h"
#include <vector>
#include <string>

class TEnv;
class TAxis;
class NPVBeamspotCorrection;

class ResidualOffsetCorrection : public asg::AsgMessaging
{

 public:
  ResidualOffsetCorrection();
  ResidualOffsetCorrection(const std::string& name, TEnv* config, TString jetAlgo, TString calibAreaTag, bool isData, bool dev);
  virtual ~ResidualOffsetCorrection();

  virtual StatusCode initialize();

  double GetResidualOffset ( double abseta, double mu, double NPV, int nJet, bool MuOnly, bool NOnly ) const;

 private:
  double GetResidualOffsetET(double abseta, double mu, double NPV, int nJet, bool MuOnly, bool NOnly,
                             const std::vector<double>& OffsetMu,
                             const std::vector<double>& OffsetNPV,
                             const std::vector<double>& OffsetNjet,
                             const TAxis *OffsetBins) const;

  double GetNPVBeamspotCorrection(double NPV) const;
 
 private:
  TEnv * m_config{};
  TString m_jetAlgo, m_calibAreaTag;
  bool m_dev{};
  bool m_isData{};
  static constexpr float m_GeV = 1000;

  NPVBeamspotCorrection * m_npvBeamspotCorr{};

  TString m_resOffsetDesc;
  TAxis * m_resOffsetBins{};
  bool m_applyNPVBeamspotCorrection{};
  double m_muSF{};
  double m_mu_ref{}, m_NPV_ref{}, m_nJet_ref{};
  bool m_useNjet{};

  std::vector<double> m_resOffsetMu, m_resOffsetNPV, m_resOffsetNjet;

};

#endif
