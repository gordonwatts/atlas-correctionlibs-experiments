/*
  Copyright (C) 2002-2023 CERN for the benefit of the ATLAS collaboration
*/

#ifndef JETJVTEFFICIENCY_FJVTEFFICIENCYTOOL_H
#define JETJVTEFFICIENCY_FJVTEFFICIENCYTOOL_H

#include "AsgDataHandles/ReadHandleKey.h"
#include "AsgTools/PropertyWrapper.h"
#include "JetJvtEfficiency/JvtEfficiencyToolBase.h"
#include "xAODEventInfo/EventInfo.h"

namespace CP {
    class FJvtEfficiencyTool : public JvtEfficiencyToolBase {
        ASG_TOOL_CLASS(FJvtEfficiencyTool, IJvtEfficiencyTool)
    public:
        FJvtEfficiencyTool(const std::string &name);
        virtual ~FJvtEfficiencyTool() override = default;

        virtual StatusCode initialize() override;

        virtual StatusCode sysApplySystematicVariation(const CP::SystematicSet &sys) override;

        virtual CorrectionCode
        getEfficiencyScaleFactor(const xAOD::Jet &jet, float &sf) const override;

        virtual CorrectionCode
        getInefficiencyScaleFactor(const xAOD::Jet &jet, float &sf) const override;

    private:
        SG::ReadHandleKey<xAOD::EventInfo> m_evtInfoKey{
                this, "EventInfoKey", "EventInfo", "The event info"};
        Gaudi::Property<std::string> m_wp{
                this, "WorkingPoint", "Loose", "The working point to use."};
        Gaudi::Property<std::string> m_file{
                this, "SFFile", "JetJvtEfficiency/May2020/fJvtSFFile.EMPFlow.root",
                "The file containing the SF histograms. Set to the empty string to use dummy scale "
                "factors."};
    };
} // namespace CP

#endif //> !JETJVTEFFICIENCY_FJVTEFFICIENCYTOOL_H