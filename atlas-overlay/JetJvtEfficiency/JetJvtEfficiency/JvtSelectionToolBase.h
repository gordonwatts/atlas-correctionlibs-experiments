/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

#ifndef JETJVTEFFICIENCY_JVTSELECTIONTOOLBASE_H
#define JETJVTEFFICIENCY_JVTSELECTIONTOOLBASE_H

#include "AsgDataHandles/ReadDecorHandleKey.h"
#include "AsgDataHandles/WriteDecorHandleKey.h"
#include "AsgTools/AsgTool.h"
#include "AsgTools/PropertyWrapper.h"
#include "PATCore/IAsgSelectionTool.h"
#include "xAODJet/JetContainer.h"
#include "JetInterface/IJetDecorator.h"

namespace CP {
    class JvtSelectionToolBase : public asg::AsgTool, virtual public IAsgSelectionTool, virtual public IJetDecorator {
    public:
        using asg::AsgTool::AsgTool;
        virtual ~JvtSelectionToolBase() = default;

        virtual StatusCode initialize() override;

        virtual const asg::AcceptInfo &getAcceptInfo() const override;

        virtual asg::AcceptData accept(const xAOD::IParticle *jet) const override;

        virtual StatusCode decorate(const xAOD::JetContainer& jets) const override;

    protected:
        Gaudi::Property<float> m_minPtForJvt{
                this, "MinPtForJvt", 20e3, "Accept all jets with pT below this"};
        Gaudi::Property<float> m_maxPtForJvt{
                this, "MaxPtForJvt", 60e3, "Accept all jets with pT above this"};
        Gaudi::Property<float> m_minEta{
                this, "MinEtaForJvt", -1, "Accept all jets with |eta| below this"};
        Gaudi::Property<float> m_maxEta{
                this, "MaxEtaForJvt", 2.5, "Accept all jets with |eta| above this"};
        // NB: Use a string not a read handle key as this is not written with a write handle key
        Gaudi::Property<std::string> m_jetEtaName{
                this, "JetEtaName", "eta", "The name of the jet eta to use."};

        SG::ReadHandleKey<xAOD::JetContainer> m_jetContainer{
                this, "JetContainer", "", "The name of the jet container"};
        SG::ReadDecorHandleKey<xAOD::JetContainer> m_jvtMomentKey{
                this, "JvtMomentName", m_jetContainer, "", "The name of the Jvt moment to use"};
        SG::WriteDecorHandleKey<xAOD::JetContainer> m_passJvtKey{
                this, "PassFlagName", m_jetContainer, "", "SG key for output pass-JVT decoration"};


        // The template AcceptInfo object
        asg::AcceptInfo m_info;
        // The index to set in the info. I suspect that this is always 0 but better to be safe
        int m_cutPos = 0;
        // The accessor for the jet eta
        SG::ConstAccessor<float> m_etaAcc { m_jetEtaName };
        // Check the range
        virtual bool isInRange(const xAOD::IParticle *jet) const;
        // Check the score
        virtual bool select(const xAOD::IParticle *jet) const = 0;

    };
} // namespace CP

#endif //> !JETJVTEFFICIENCY_JVTSELECTIONTOOL_H
