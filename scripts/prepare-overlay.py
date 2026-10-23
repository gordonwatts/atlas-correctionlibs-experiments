"""Generate a release-specific source overlay; leave the CVMFS reference untouched."""
from pathlib import Path
import shutil
r=Path(__file__).resolve().parents[1]
release=Path("/cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt/src")
dest=r/"atlas-overlay"
packages=["JetCalibTools","JetUncertainties","JetJvtEfficiency"]
def replace_body(text,signature,body):
 start=text.index(signature);left=text.index("{",start);depth=1;i=left+1
 while depth:
  if text[i]=="{":depth+=1
  elif text[i]=="}":depth-=1
  i+=1
 return text[:left+1]+"\n"+body+"\n"+text[i-1:]
for name in packages:
 src=release/"Reconstruction/Jet"/name;out=dest/name
 if out.exists():shutil.rmtree(out)
 shutil.copytree(src,out)
 cm=out/"CMakeLists.txt"
 cm.write_text(cm.read_text()+f"""
# CorrectionLib demo overlay, generated from the pinned release.
find_package(correctionlib CONFIG REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(lwtnn REQUIRED)
target_include_directories({name+"Lib"} PRIVATE "{r}/src" ${{LWTNN_INCLUDE_DIRS}})
target_link_libraries({name+"Lib"} PRIVATE correctionlib Eigen3::Eigen ${{LWTNN_LIBRARIES}})
""")
# Nominal calibration: retain ATLAS event-input handling and original public properties.
h=dest/"JetCalibTools/JetCalibTools/JetCalibrationTool.h";s=h.read_text()
s="class Nominal;\n"+s
s=s.replace("private:","private:\n  std::shared_ptr<::Nominal> m_correctionBackend;",1);h.write_text(s)
p=dest/"JetCalibTools/Root/JetCalibrationTool.cxx";s=p.read_text()
s='#include "adapter_support.hxx"\n'+s
needle="  //Loop over the request calib sequence"
end=s.index("  // Initialise ReadHandle(s)",s.index(needle))
s=s[:s.index(needle)]+"""
  if(m_isData || m_jetAlgo!="AntiKt4EMPFlow" || m_calibSeq!="JetArea_Residual_EtaJES_GSC" ||
     m_calibAreaTag!="00-04-83" ||
     m_config!="AntiKt4EMPFlow_MC23a_PreRecR22_Phase2_CalibConfig_ResPU_EtaJES_GSC_241208_InSitu.config" ||
     (m_gscDepth!="auto" && m_gscDepth!="trackWIDTH")) {
    ATH_MSG_ERROR("No validated CorrectionLib bundle for these calibration properties");
    return StatusCode::FAILURE;
  }
  try { m_correctionBackend=atlasNominalBackend(); }
  catch(const std::exception& e){ATH_MSG_ERROR(e.what());return StatusCode::FAILURE;}
"""+s[end:]
s=s.replace("if( m_calibSteps.empty() )","if( !m_correctionBackend )")
old="  for (unsigned int i=0; i<m_calibSteps.size(); ++i) ATH_CHECK(m_calibSteps[i]->calibrate(jet, jetEventInfo));"
new="""
  try {
    const auto p=jet.getAttribute<xAOD::JetFourMom_t>("JetConstitScaleMomentum");
    const size_t pv=jetEventInfo.PVIndex();
    const auto sampling=jet.getAttribute<std::vector<float>>("EnergyPerSampling");
    const float energy=p.E();
    const double tile=(sampling.at(12)+sampling.at(18))/energy;
    const double em3=(sampling.at(3)+sampling.at(7))/energy;
    const auto charged=jet.getAttribute<std::vector<float>>("SumPtChargedPFOPt500");
    const auto ntrk=jet.getAttribute<std::vector<int>>("NumTrkPt1000");
    const auto width=jet.getAttribute<std::vector<float>>("TrackWidthPt1000");
    auto out=m_correctionBackend->evaluate({p.Pt(),p.Eta(),p.Phi(),p.M(),jet.getAttribute<float>("DetectorEta"),
      jet.getAttribute<xAOD::JetFourMom_t>("ActiveArea4vec").Pt(),jetEventInfo.rho(),jetEventInfo.mu(),double(jetEventInfo.NPV()),
      charged.at(pv)/p.Pt(),tile,em3,double(ntrk.at(pv)),width.at(pv)});
    const std::vector<std::string> stages={"JetPileupScaleMomentum","JetEtaJESScaleMomentum","JetGSCScaleMomentum"};
    for(size_t i=0;i<3;++i)jet.setAttribute<xAOD::JetFourMom_t>(stages[i],xAOD::JetFourMom_t(out[4*i],out[4*i+1],out[4*i+2],out[4*i+3]));
    jet.setJetP4(xAOD::JetFourMom_t(out[8],out[9],out[10],out[11]));
    jet.setAttribute<int>("PileupCorrected",true);
  }catch(const std::exception& e){ATH_MSG_ERROR(e.what());return StatusCode::FAILURE;}
"""
assert old in s;s=s.replace(old,new);p.write_text(s)
# JES/JER: preserve CP registration and the original EventInfo/NPV preparation.
h=dest/"JetUncertainties/JetUncertainties/JetUncertaintiesTool.h";s=h.read_text()
s="#include <memory>\nclass JetSystematics;\n"+s
s=s.replace("private:","private:\n        std::shared_ptr<::JetSystematics> m_correctionBackend;",1);h.write_text(s)
p=dest/"JetUncertainties/Root/JetUncertaintiesTool.cxx";s='#include "adapter_support.hxx"\n'+p.read_text()
s=replace_body(s,"StatusCode JetUncertaintiesTool::initialize()", """
 if(m_isInit)return StatusCode::FAILURE;
 if(m_jetDef!="AntiKt4EMPFlow" || m_mcType!="MC23" ||
    m_configFile!="rel22/Winter2025_PreRec/R4_CategoryReduction_FullJER.config" || m_userSeed!=0 || !m_systFilters.empty()){
  ATH_MSG_ERROR("No validated CorrectionLib bundle for these uncertainty properties");return StatusCode::FAILURE;
 }
 try{
  auto root=atlasCorrectionBundle();
  m_correctionBackend=std::make_shared<::JetSystematics>(root+"/uncertainty-tables.json.gz",root+"/uncertainty-manifest.json");
  std::set<std::string> bases;
  for(const auto& name:m_correctionBackend->names()){
   if(name=="NOSYS")continue;
   if(name.find("_PseudoData")!=std::string::npos&&!m_pseudoDataJERsmearingMode)continue;
   bases.insert(name.substr(0,name.find("__")));
  }
  for(const auto& name:bases)ATH_CHECK(addAffectingSystematic(CP::SystematicVariation(name,CP::SystematicVariation::CONTINUOUS),true));
 }catch(const std::exception& e){ATH_MSG_ERROR(e.what());return StatusCode::FAILURE;}
 m_isInit=true;return asg::AsgTool::initialize();
""")
s=replace_body(s,"StatusCode JetUncertaintiesTool::applySystematicVariation(const CP::SystematicSet& systConfig)", """
 CP::SystematicSet filtered;
 ATH_CHECK(getFilteredSystematicSet(systConfig,filtered));
 if(filtered.size()>1){ATH_MSG_ERROR("Only individual nuisance variations are validated");return StatusCode::FAILURE;}
 m_currentSystSet.swap(filtered);return StatusCode::SUCCESS;
""")
s=replace_body(s,"CP::CorrectionCode JetUncertaintiesTool::applyCorrection(xAOD::Jet& jet, const xAOD::EventInfo& eInfo) const", """
 if(!m_isInit||!m_correctionBackend)return CP::CorrectionCode::Error;
 try{
  static const SG::AuxElement::ConstAccessor<float> npv("NPV");
  static const SG::AuxElement::ConstAccessor<char> bjet("IsBjet");
  std::string sys=m_currentSystSet.name();if(sys.empty())sys="NOSYS";
  if(m_isData&&sys.find("JER")!=std::string::npos&&sys.find("_PseudoData")==std::string::npos)sys.insert(sys.find("__"),"_PseudoData");
  auto out=m_correctionBackend->evaluate(sys,{jet.pt(),jet.eta(),jet.phi(),jet.m(),eInfo.averageInteractionsPerCrossing(),npv(eInfo),bjet.isAvailable(jet)?double(bjet(jet)):0.});
  jet.setJetP4(xAOD::JetFourMom_t(out[0],out[1],out[2],out[3]));return CP::CorrectionCode::Ok;
 }catch(const std::exception& e){ATH_MSG_ERROR(e.what());return CP::CorrectionCode::Error;}
""");p.write_text(s)
# Efficiency: preserve truth/validity wrappers and CP interface, replace numerical lookup.
h=dest/"JetJvtEfficiency/JetJvtEfficiency/JvtEfficiencyToolBase.h";s=h.read_text()
s="#include <memory>\nclass JvtEfficiency;\n"+s
s=s.replace("private:","private:\n        std::shared_ptr<::JvtEfficiency> m_correctionBackend;",1)
if "m_correctionBackend" not in s:s=s.replace("protected:","protected:\n        std::shared_ptr<::JvtEfficiency> m_correctionBackend;",1)
h.write_text(s)
p=dest/"JetJvtEfficiency/Root/JvtEfficiencyToolBase.cxx";s='#include "adapter_support.hxx"\n'+p.read_text()
s=replace_body(s,"StatusCode JvtEfficiencyToolBase::initHists(", """
 if(file!="JetJvtEfficiency/May2024/NNJvtSFFile_Run3_EMPFlow.root"||wp!="FixedEffPt"){
  ATH_MSG_ERROR("No validated CorrectionLib bundle for these JVT properties");return StatusCode::FAILURE;
 }
 try{auto root=atlasCorrectionBundle();m_correctionBackend=std::make_shared<::JvtEfficiency>(root+"/uncertainty-tables.json.gz",root+"/uncertainty-manifest.json");}
 catch(const std::exception& e){ATH_MSG_ERROR(e.what());return StatusCode::FAILURE;}
 m_useDummySFs=false;return StatusCode::SUCCESS;
""")
for method,passed in [("getEffImpl","true"),("getIneffImpl","false")]:
 s=replace_body(s,"CorrectionCode JvtEfficiencyToolBase::"+method+"(",f"""
 try{{auto result=m_correctionBackend->evaluate(x,y,{passed},m_appliedSysSigma);sf=result[0];return result[1]==0?CorrectionCode::Ok:CorrectionCode::OutOfValidityRange;}}
 catch(const std::exception& e){{ATH_MSG_ERROR(e.what());return CorrectionCode::Error;}}
""")
p.write_text(s)
(dest/"CMakeLists.txt").write_text("""cmake_minimum_required(VERSION 3.25)
project(CorrectionLibOverlay VERSION 1.0.0 LANGUAGES CXX)
find_package(AnalysisBase 25.2.100 EXACT REQUIRED)
atlas_project(USE AnalysisBase 25.2.100)
atlas_cpack_setup()
""")
print("Prepared release-specific adapters",packages)
