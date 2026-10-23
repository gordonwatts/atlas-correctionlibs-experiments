/*
  Copyright (C) 2002-2023 CERN for the benefit of the ATLAS collaboration
*/

#ifndef JETJVTEFFICIENCY_JVTEFFICIENCYTOOL_H
#define JETJVTEFFICIENCY_JVTEFFICIENCYTOOL_H

#include "AsgTools/PropertyWrapper.h"
#include "JetJvtEfficiency/JvtEfficiencyToolBase.h"

namespace CP {
    class JvtEfficiencyTool : public JvtEfficiencyToolBase {
        ASG_TOOL_CLASS(JvtEfficiencyTool, IJvtEfficiencyTool)
    public:
        using JvtEfficiencyToolBase::JvtEfficiencyToolBase;
        virtual ~JvtEfficiencyTool() override = default;

        virtual StatusCode initialize() override;

        virtual StatusCode sysApplySystematicVariation(const CP::SystematicSet &sys) override;

    private:
        Gaudi::Property<bool> m_isPFlow{
                this, "IsPFlow", true,
                "Whether the jet collection is PFlow or not. Used to configure the correct working "
                "points"};
        Gaudi::Property<std::string> m_wp{
                this, "WorkingPoint", "Default", "The working point to use."};
        Gaudi::Property<std::string> m_file{
                this, "SFFile", "Default",
                "The file containing the SF histograms. Set to the empty string to use dummy scale "
                "factors."};
    };
} // namespace CP

#endif //> !JETJVTEFFICIENCY_NNJVTEFFICIENCYTOOL_H