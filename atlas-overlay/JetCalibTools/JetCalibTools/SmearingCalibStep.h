///////////////////////// -*- C++ -*- /////////////////////////////

/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

// SmearingCalibStep.h 
// Header file for class SmearingCalibStep
/////////////////////////////////////////////////////////////////// 

#ifndef JETCALIBTOOLS_SMEARINGCALIBSTEP_H
#define JETCALIBTOOLS_SMEARINGCALIBSTEP_H

#include "AsgTools/AsgTool.h"
#include "AsgTools/AsgToolMacros.h"
#include "AsgTools/ToolHandle.h"
#include "AsgTools/PropertyWrapper.h"

#include "xAODEventInfo/EventInfo.h"

#include "JetAnalysisInterfaces/IJetCalibTool.h"
#include "JetAnalysisInterfaces/IJetCalibStep.h"
#include "JetAnalysisInterfaces/IVarTool.h"

#include "TRandom3.h"
#include "boost/thread/tss.hpp"

class SmearingCalibStep
  : public asg::AsgTool,
      virtual public IJetCalibStep {

        ASG_TOOL_CLASS(SmearingCalibStep, IJetCalibStep)

    public:
        SmearingCalibStep(const std::string& name = "SmearingCalibStep");
  
        virtual StatusCode initialize() override;
        virtual StatusCode calibrate(xAOD::JetContainer&) const override;
  
        virtual StatusCode getNominalResolutionData(const xAOD::Jet& jet, const JetHelper::JetContext& jc, double& resolution) const override;
        virtual StatusCode getNominalResolutionMC(const xAOD::Jet& jet,  const JetHelper::JetContext& jc, double& resolution) const override;

    private:
        Gaudi::Property<std::string> m_jetInScale {this, "InScale", "JetGSCScaleMomentum", "Starting jet scale"};
        Gaudi::Property<std::string> m_jetOutScale {this, "OutScale", "JetSmearedMomentum", "Ending jet scale"};
        Gaudi::Property<std::string> m_smearType {this, "SmearType", "FourVec", "How to smear: pt, eta or FourVec"};

        ToolHandle<JetHelper::IVarTool> m_histToolMC {this, "HistoReaderMC", "HistoInput2D", "Instance of HistoInput1D or HistoInput2D for reading histogram for MC"};
        ToolHandle<JetHelper::IVarTool> m_histToolData {this, "HistoReaderData", "HistoInput2D", "Instance of HistoInput1D or HistoInput2D for reading histogram for data"};

        // Helper methods
        StatusCode getSigmaSmear(xAOD::Jet& jet, const JetHelper::JetContext & jc, double& sigmaSmear) const;
        StatusCode getNominalResolution(const xAOD::Jet& jet, JetHelper::JetContext jc, double& resolution) const;
        TRandom3* getTLSRandomGen(unsigned long seed) const;

        // Private enums
        enum class SmearType
        {
            UNKNOWN=0,  // Unknown/unset/etc
            Pt,         // pT smearing (JpTR)
            Mass,       // Mass smearing (JMR)
            FourVec     // Four-vector smearing (JER)
        };
        SmearType m_smearTypeClass = SmearType::UNKNOWN;

        mutable boost::thread_specific_ptr<TRandom3> m_rand_tls; // thread-specific random number generator
};

#endif
