/*
  Copyright (C) 2002-2023 CERN for the benefit of the ATLAS collaboration
*/

#include "JetJvtEfficiency/NNJvtEfficiencyTool.h"

namespace CP {

    StatusCode NNJvtEfficiencyTool::initialize() {
        ATH_CHECK(JvtEfficiencyToolBase::initialize());
        ATH_CHECK(initHists(m_file, m_wp));
        if (!addAffectingSystematic(NNJvtEfficiencyUp, true) ||
            !addAffectingSystematic(NNJvtEfficiencyDown, true)) {
            ATH_MSG_ERROR("failed to set up NNJvt systematics");
            return StatusCode::FAILURE;
        }
        return StatusCode::SUCCESS;
    }

    StatusCode NNJvtEfficiencyTool::sysApplySystematicVariation(const SystematicSet &sys) {
        if (sys.find(NNJvtEfficiencyUp) != sys.end())
            m_appliedSysSigma = 1;
        else if (sys.find(NNJvtEfficiencyDown) != sys.end())
            m_appliedSysSigma = -1;
        else
            m_appliedSysSigma = 0;
        return StatusCode::SUCCESS;
    }
} // namespace CP