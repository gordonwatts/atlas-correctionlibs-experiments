/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

#ifndef JETJVTEFFICIENCY_JVTSELECTIONTOOL_H
#define JETJVTEFFICIENCY_JVTSELECTIONTOOL_H

#include "AsgTools/PropertyWrapper.h"
#include "JetJvtEfficiency/JvtSelectionToolBase.h"


namespace CP {
    class JvtSelectionTool : public JvtSelectionToolBase {
        ASG_TOOL_CLASS(JvtSelectionTool, IAsgSelectionTool)
    public:
        using JvtSelectionToolBase::JvtSelectionToolBase;
        virtual ~JvtSelectionTool() override = default;

        virtual StatusCode initialize() override;

    private:
        Gaudi::Property<bool> m_isPFlow{
                this, "IsPFlow", true,
                "Whether the jet collection is PFlow or not. Used to configure the correct working "
                "points"};
        Gaudi::Property<std::string> m_wp{
                this, "WorkingPoint", "Default",
                "The working point to use. Set to 'Custom' to manually set the values"};
        Gaudi::Property<float> m_jvtCutBorder{
                this, "JvtCutBorder", -1,
                "The JVT selection to make in the border region (2.4-2.5)"};
        Gaudi::Property<float> m_jvtCut{this, "JvtCut", -1, "The JVT selection to make"};

        virtual bool select(const xAOD::IParticle *jet) const override;

    };
} // namespace CP

#endif //> !JETJVTEFFICIENCY_JVTSELECTIONTOOL_H
