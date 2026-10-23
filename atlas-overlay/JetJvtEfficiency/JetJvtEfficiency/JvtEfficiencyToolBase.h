#include <memory>
class JvtEfficiency;
/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

#ifndef JETJVTEFFICIENCY_JVTEFFICIENCYTOOLBASE_H
#define JETJVTEFFICIENCY_JVTEFFICIENCYTOOLBASE_H

#include "AsgDataHandles/ReadDecorHandle.h"
#include "AsgTools/AsgTool.h"
#include "AsgTools/PropertyWrapper.h"
#include "JetAnalysisInterfaces/IJvtEfficiencyTool.h"
#include "PATInterfaces/SystematicsTool.h"

#include <TH2.h>
#include <memory>

namespace CP {
    class JvtEfficiencyToolBase : public asg::AsgTool,
                                  public CP::SystematicsTool,
                                  virtual public CP::IJvtEfficiencyTool {
    public:
        using asg::AsgTool::AsgTool;
        virtual ~JvtEfficiencyToolBase() override = default;

        virtual StatusCode initialize() override;

        virtual CorrectionCode
        getEfficiencyScaleFactor(const xAOD::Jet &jet, float &sf) const override;

        virtual CorrectionCode
        getInefficiencyScaleFactor(const xAOD::Jet &jet, float &sf) const override;

    protected:
        std::shared_ptr<::JvtEfficiency> m_correctionBackend;
        Gaudi::Property<std::string> m_jetContainer{
                this, "JetContainer", "",
                "The name of the jet container, used to correctly initialize the read handles"};
        Gaudi::Property<bool> m_doTruthRequirement{
                this, "DoTruthReq", true,
                "Use the truth-matching requirement. **Strongly** recommended"};
        SG::ReadDecorHandleKey<xAOD::JetContainer> m_truthHSLabel{
                this, "TruthHSLabel", "isJvtHS", "Label for truth-matched jets"};
        Gaudi::Property<float> m_minEta{
                this, "MinEtaForJvt", -1, "All jets with |eta| below this are out of range"};
        Gaudi::Property<float> m_maxEta{
                this, "MaxEtaForJvt", 2.5, "All jets with |eta| above this are out of range"};
        Gaudi::Property<float> m_minPtForJvt{
                this, "MinPtForJvt", 20e3, "All jets with pT below this are out of range"};
        Gaudi::Property<float> m_maxPtForJvt{
                this, "MaxPtForJvt", 60e3, "All jets with pT above this are out of range"};
        Gaudi::Property<float> m_dummySFError{
                this, "DummySFError", 0.1, "The amount by which to vary the dummy SF"};
        // NB: Use a string not a read handle key as this is not written with a write handle key
        Gaudi::Property<std::string> m_jetEtaName{
                this, "JetEtaName", "eta", "The name of the jet eta to use."};
        std::unique_ptr<TH2> m_jvtHist;
        std::unique_ptr<TH2> m_effHist;
        bool m_useDummySFs{false};
        // The accessor for the jet eta
        SG::ConstAccessor<float> m_etaAcc { m_jetEtaName };
        // -1, 0 or +1 depending on the systematic
        int m_appliedSysSigma = 0;
        // TEMPORARY: Allow for using an accessor rather than the full decorhandle
        std::optional<SG::AuxElement::ConstAccessor<char>> m_accIsHS;

        /// Read the input histograms. Passing an empty 'file' string uses dummy SFs
        StatusCode initHists(const std::string &file, const std::string &wp);
        bool isInRange(const xAOD::Jet &jet) const;
        CorrectionCode getEffImpl(float x, float y, float &sf) const;
        CorrectionCode getIneffImpl(float x, float y, float &sf) const;
    };
} // namespace CP

#endif //> !JETJVTEFFICIENCY_JVTEFFICIENCYTOOLBASE_H
