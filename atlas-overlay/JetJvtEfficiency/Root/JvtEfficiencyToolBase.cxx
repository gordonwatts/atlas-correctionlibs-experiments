#include "adapter_support.hxx"
/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

#include "JetJvtEfficiency/JvtEfficiencyToolBase.h"
#include "AsgDataHandles/ReadDecorHandle.h"
#include "PathResolver/PathResolver.h"

#include <TFile.h>

namespace {
    bool getBin(const TAxis &axis, float value, int &bin) {
        bin = axis.FindBin(value);
        return (bin != 0 && bin != axis.GetNbins()+1);
    }
    bool getBinContentAndError(const TH2 &h, float x, float y, float &content, float &error) {
        int xBin{}, yBin{};
        if (!getBin(*h.GetXaxis(), x, xBin) || !getBin(*h.GetYaxis(), y, yBin)) {

            content = -1;
            error = -1;
            return false;
        }
        content = h.GetBinContent(xBin, yBin);
        error = h.GetBinError(xBin, yBin);
        return true;
    }
} // namespace

namespace CP {
    StatusCode JvtEfficiencyToolBase::initialize() {
        m_etaAcc = SG::ConstAccessor<float>(m_jetEtaName);
        if (!m_doTruthRequirement)
            ATH_MSG_WARNING("No truth requirement will be performed, which is not recommended.");
        m_accIsHS.emplace(m_truthHSLabel.key());
        if (m_jetContainer.empty()) {
            ATH_MSG_WARNING("No JetContainer set. This behaviour is deprecated");
            ATH_CHECK(m_truthHSLabel.initialize(false));
        }
        else {
            m_truthHSLabel = m_jetContainer + "." + m_truthHSLabel.key();
            ATH_CHECK(m_truthHSLabel.initialize(m_doTruthRequirement));
        }
        return StatusCode::SUCCESS;
    }

    CorrectionCode
    JvtEfficiencyToolBase::getEfficiencyScaleFactor(const xAOD::Jet &jet, float &sf) const {
        if (!isInRange(jet)) {
            sf = -1;
            return CorrectionCode::OutOfValidityRange;
        }
        if (m_doTruthRequirement) {
            if (!m_accIsHS->isAvailable(jet)) {
                ATH_MSG_ERROR("Truth tagging required but not available");
                return CorrectionCode::Error;
            }
            if (!(*m_accIsHS)(jet)) {
                sf = 1;
                return CorrectionCode::Ok;
            }
        }
        return getEffImpl(jet.pt(), m_etaAcc(jet), sf);
    }

    CorrectionCode
    JvtEfficiencyToolBase::getInefficiencyScaleFactor(const xAOD::Jet &jet, float &sf) const {
        if (!isInRange(jet)) {
            sf = -1;
            return CorrectionCode::OutOfValidityRange;
        }
        if (m_doTruthRequirement) {
            if (!m_accIsHS->isAvailable(jet)) {
                ATH_MSG_ERROR("Truth tagging required but not available");
                return CorrectionCode::Error;
            }
            if (!(*m_accIsHS)(jet)) {
                sf = 1;
                return CorrectionCode::Ok;
            }
        }
        return getIneffImpl(jet.pt(), m_etaAcc(jet), sf);
    }

    StatusCode JvtEfficiencyToolBase::initHists(const std::string &file, const std::string &wp) {

 if(file!="JetJvtEfficiency/May2024/NNJvtSFFile_Run3_EMPFlow.root"||wp!="FixedEffPt"){
  ATH_MSG_ERROR("No validated CorrectionLib bundle for these JVT properties");return StatusCode::FAILURE;
 }
 try{auto root=atlasCorrectionBundle();m_correctionBackend=std::make_shared<::JvtEfficiency>(root+"/uncertainty-tables.json.gz",root+"/uncertainty-manifest.json");}
 catch(const std::exception& e){ATH_MSG_ERROR(e.what());return StatusCode::FAILURE;}
 m_useDummySFs=false;return StatusCode::SUCCESS;

}

    CorrectionCode JvtEfficiencyToolBase::getEffImpl(float x, float y, float &sf) const {

 try{auto result=m_correctionBackend->evaluate(x,y,true,m_appliedSysSigma);sf=result[0];return result[1]==0?CorrectionCode::Ok:CorrectionCode::OutOfValidityRange;}
 catch(const std::exception& e){ATH_MSG_ERROR(e.what());return CorrectionCode::Error;}

}

    CorrectionCode JvtEfficiencyToolBase::getIneffImpl(float x, float y, float &sf) const {

 try{auto result=m_correctionBackend->evaluate(x,y,false,m_appliedSysSigma);sf=result[0];return result[1]==0?CorrectionCode::Ok:CorrectionCode::OutOfValidityRange;}
 catch(const std::exception& e){ATH_MSG_ERROR(e.what());return CorrectionCode::Error;}

}

    bool JvtEfficiencyToolBase::isInRange(const xAOD::Jet &jet) const {
        if (jet.pt() < m_minPtForJvt || jet.pt() > m_maxPtForJvt)
            return false;
        float eta = m_etaAcc(jet);
        return std::abs(eta) >= m_minEta && std::abs(eta) <= m_maxEta;
    }
} // namespace CP
