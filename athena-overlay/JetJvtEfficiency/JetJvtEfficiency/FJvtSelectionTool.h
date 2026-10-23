/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

#ifndef JETJVTEFFICIENCY_FJVTSELECTIONTOOL_H
#define JETJVTEFFICIENCY_FJVTSELECTIONTOOL_H

#include "AsgTools/PropertyWrapper.h"
#include "JetJvtEfficiency/JvtSelectionToolBase.h"

namespace CP {
    class FJvtSelectionTool : public JvtSelectionToolBase {
        ASG_TOOL_CLASS(FJvtSelectionTool, IAsgSelectionTool)
    public:
        FJvtSelectionTool(const std::string &name);
        virtual ~FJvtSelectionTool() override = default;

        virtual StatusCode initialize() override;

    private:
        Gaudi::Property<std::string> m_wp{
                this, "WorkingPoint", "Loose",
                "The working point to use. Set to 'Custom' to manually set the values"};
        Gaudi::Property<float> m_jvtCut{this, "JvtCut", 999, "The JVT selection to make"};
        
        SG::ReadDecorHandleKey<xAOD::JetContainer> m_timingKey{
                this, "TimingMomentName", m_jetContainer, "Timing", "The name of the timing moment to use"};
        Gaudi::Property<float> m_timingCut{
                this, "TimingCut", -1, "Only accept jets with time less than this; negative values deactivate timing requirement"};

        virtual bool select(const xAOD::IParticle *jet) const override;
    };
} // namespace CP

#endif //> !JETJVTEFFICIENCY_JVTSELECTIONTOOL_H
