#include "JetJvtEfficiency/FJvtSelectionTool.h"
#include "AsgDataHandles/ReadDecorHandle.h"

namespace {
    const static std::map<std::string, float> workingPoints{{"Loose", 0.5}, {"Tight", 0.4}, {"Tighter", 0.2}};
}

namespace CP {

    FJvtSelectionTool::FJvtSelectionTool(const std::string &name) : JvtSelectionToolBase(name) {
        m_minEta = 2.5;
        m_maxEta = 4.5;
    }

    StatusCode FJvtSelectionTool::initialize() {

        ATH_CHECK(JvtSelectionToolBase::initialize());

        ATH_CHECK(m_timingKey.initialize());

        if (m_wp != "Custom") {
            auto itr = workingPoints.find(m_wp);
            if (itr == workingPoints.end()) {
                ATH_MSG_ERROR("Invalid fJvt working point name");
                return StatusCode::FAILURE;
            }
            m_jvtCut = itr->second;
        }

        return StatusCode::SUCCESS;
    }

    bool FJvtSelectionTool::select(const xAOD::IParticle *jet) const {
        if(!isInRange(jet)) return true;
        // select jet if it passes fJvt requirement and timing cut (if configured)
        SG::ReadDecorHandle<xAOD::JetContainer, float> jvtHandle(m_jvtMomentKey);
        SG::ReadDecorHandle<xAOD::JetContainer, float> timingHandle(m_timingKey);
        return jvtHandle(*jet) <= m_jvtCut && ( m_timingCut > 0 ? std::abs( timingHandle(*jet) ) <= m_timingCut : true );
    }

} // namespace CP
