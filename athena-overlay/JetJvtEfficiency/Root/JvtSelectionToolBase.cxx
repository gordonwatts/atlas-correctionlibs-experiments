/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

#include "JetJvtEfficiency/JvtSelectionToolBase.h"
#include "AsgDataHandles/ReadDecorHandle.h"
#include "AsgDataHandles/WriteDecorHandle.h"

namespace CP {
    StatusCode JvtSelectionToolBase::initialize() {
        m_etaAcc = SG::ConstAccessor<float>(m_jetEtaName);
        m_cutPos = m_info.addCut("Jvt", "Whether the jet passes the Jvt selection");

        ATH_CHECK(m_jetContainer.initialize());
        ATH_CHECK(m_jvtMomentKey.initialize());
        ATH_CHECK(m_passJvtKey.initialize(SG::AllowEmpty));

        return StatusCode::SUCCESS;
    }

    const asg::AcceptInfo &JvtSelectionToolBase::getAcceptInfo() const { return m_info; }

    asg::AcceptData JvtSelectionToolBase::accept(const xAOD::IParticle *jet) const {
        asg::AcceptData data(&m_info);
        data.setCutResult(m_cutPos, select(jet));
        return data;
    }

    bool JvtSelectionToolBase::isInRange(const xAOD::IParticle *jet) const {
        if (jet->pt() < m_minPtForJvt || jet->pt() > m_maxPtForJvt)
            return false;
        float eta = m_etaAcc(*jet);
        return std::abs(eta) >= m_minEta && std::abs(eta) <= m_maxEta;
    }

    StatusCode JvtSelectionToolBase::decorate(const xAOD::JetContainer& jets) const {
        if(m_passJvtKey.key().empty()){
            ATH_MSG_ERROR("Attempted to decorate jets with JVT decision, but decoration name was not configured!");
            return StatusCode::FAILURE;
        }
        SG::WriteDecorHandle<xAOD::JetContainer, char>  passJvtHandle(m_passJvtKey);
        for(const xAOD::Jet* jet : jets) passJvtHandle(*jet) = select(jet);
        return StatusCode::SUCCESS;
    }

} // namespace CP
