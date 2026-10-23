/*
  Copyright (C) 2002-2023 CERN for the benefit of the ATLAS collaboration
*/

#include "JetJvtEfficiency/JvtEfficiencyTool.h"

namespace CP {

    StatusCode JvtEfficiencyTool::initialize() {
        ATH_CHECK(JvtEfficiencyToolBase::initialize());
        if (m_wp == "Default")
            m_wp = m_isPFlow ? "Tight" : "Medium";
        if (m_file == "Default")
            m_file = "JetJvtEfficiency/Moriond2018/JvtSFFile_" +
                     std::string(m_isPFlow ? "EMPFlow" : "EMTopo") + "Jets.root";
        ATH_CHECK(initHists(m_file, m_wp));
        if (!addAffectingSystematic(JvtEfficiencyUp, true) ||
            !addAffectingSystematic(JvtEfficiencyDown, true)) {
            ATH_MSG_ERROR("failed to set up NNJvt systematics");
            return StatusCode::FAILURE;
        }
        return StatusCode::SUCCESS;
    }

    StatusCode JvtEfficiencyTool::sysApplySystematicVariation(const SystematicSet &sys) {
        if (sys.find(JvtEfficiencyUp) != sys.end())
            m_appliedSysSigma = 1;
        else if (sys.find(JvtEfficiencyDown) != sys.end())
            m_appliedSysSigma = -1;
        else
            m_appliedSysSigma = 0;
        return StatusCode::SUCCESS;
    }
} // namespace CP