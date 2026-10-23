/*
  Copyright (C) 2002-2023 CERN for the benefit of the ATLAS collaboration
*/

#include "JetJvtEfficiency/FJvtEfficiencyTool.h"

#include "AsgDataHandles/ReadHandle.h"
#include "AsgDataHandles/ReadDecorHandle.h"

namespace CP {
    FJvtEfficiencyTool::FJvtEfficiencyTool(const std::string &name) : JvtEfficiencyToolBase(name) {
        m_minEta = 2.5;
        m_maxEta = 4.5;
    }

    StatusCode FJvtEfficiencyTool::initialize() {
        ATH_CHECK(JvtEfficiencyToolBase::initialize());
        ATH_CHECK(m_evtInfoKey.initialize());
        ATH_CHECK(initHists(m_file, m_wp));
        if (!addAffectingSystematic(fJvtEfficiencyUp, true) ||
            !addAffectingSystematic(fJvtEfficiencyDown, true)) {
            ATH_MSG_ERROR("failed to set up fJvt systematics");
            return StatusCode::FAILURE;
        }
        return StatusCode::SUCCESS;
    }

    StatusCode FJvtEfficiencyTool::sysApplySystematicVariation(const SystematicSet &sys) {
        if (sys.find(fJvtEfficiencyUp) != sys.end())
            m_appliedSysSigma = 1;
        else if (sys.find(fJvtEfficiencyDown) != sys.end())
            m_appliedSysSigma = -1;
        else
            m_appliedSysSigma = 0;
        return StatusCode::SUCCESS;
    }

    CorrectionCode
    FJvtEfficiencyTool::getEfficiencyScaleFactor(const xAOD::Jet &jet, float &sf) const {
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
        auto evtInfo = SG::makeHandle(m_evtInfoKey);
        if (!evtInfo.isValid()) {
            ATH_MSG_ERROR("Failed to retrieve " << m_evtInfoKey.key());
            return CorrectionCode::Error;
        }
        return getEffImpl(jet.pt(), evtInfo->actualInteractionsPerCrossing(), sf);
    }

    CorrectionCode
    FJvtEfficiencyTool::getInefficiencyScaleFactor(const xAOD::Jet &jet, float &sf) const {
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
        auto evtInfo = SG::makeHandle(m_evtInfoKey);
        if (!evtInfo.isValid()) {
            ATH_MSG_ERROR("Failed to retrieve " << m_evtInfoKey.key());
            return CorrectionCode::Error;
        }
        return getIneffImpl(jet.pt(), evtInfo->actualInteractionsPerCrossing(), sf);
    }
} // namespace CP