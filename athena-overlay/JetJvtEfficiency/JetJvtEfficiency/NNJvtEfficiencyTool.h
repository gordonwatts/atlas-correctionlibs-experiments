/*
  Copyright (C) 2002-2023 CERN for the benefit of the ATLAS collaboration
*/

#ifndef JETJVTEFFICIENCY_NNJVTEFFICIENCYTOOL_H
#define JETJVTEFFICIENCY_NNJVTEFFICIENCYTOOL_H

#include "AsgTools/PropertyWrapper.h"
#include "JetJvtEfficiency/JvtEfficiencyToolBase.h"

namespace CP {
    class NNJvtEfficiencyTool : public JvtEfficiencyToolBase {
        ASG_TOOL_CLASS(NNJvtEfficiencyTool, IJvtEfficiencyTool)
    public:
        using JvtEfficiencyToolBase::JvtEfficiencyToolBase;
        ~NNJvtEfficiencyTool() override = default;

        virtual StatusCode initialize() override;

        virtual StatusCode sysApplySystematicVariation(const CP::SystematicSet &sys) override;

    private:
        Gaudi::Property<std::string> m_wp{
                this, "WorkingPoint", "FixedEffPt", "The working point to use."};
        Gaudi::Property<std::string> m_file{
                this, "SFFile", "",
                "The file containing the SF histograms. Set to the empty string to use dummy scale "
                "factors."};
    };
} // namespace CP

#endif //> !JETJVTEFFICIENCY_NNJVTEFFICIENCYTOOL_H