from pathlib import Path
import shutil
r=Path('/home/gwatts/atlas/correctionlib_tests');w=Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib')
shutil.copy2(w/'nn_graph.hxx',r/'src/nn_graph.hxx')
p=r/'src/nnjvt.hxx';s=p.read_text().replace('#include <lwtnn/generic/FastGraph.hh>','#include "nn_graph.hxx"').replace('std::unique_ptr<lwt::generic::FastGraph<double>> graph;','std::unique_ptr<CachedNNGraph> graph;').replace('std::make_unique<lwt::generic::FastGraph<double>>(cfg,order)','std::make_unique<CachedNNGraph>(cfg,order)')
a=s.index('    std::vector<double> f{rpt,');b=s.index('\n   }',a);s=s[:a]+'    score=graph->score4({rpt,float(r[4]),float(ip),float(ie)});'+s[b:];p.write_text(s)
source=Path('/cvmfs/atlas.cern.ch/repo/sw/software/25.2/AnalysisBase/25.2.100/InstallArea/x86_64-el9-gcc15-opt/src/Reconstruction/Jet/JetMomentTools')
dest=r/'atlas-overlay/JetMomentTools';shutil.copytree(source,dest)
p=dest/'JetMomentTools/JetVertexNNTagger.h';s=p.read_text().replace('#include "lwtnn/generic/FastGraph.hh"','class CachedNNGraph;').replace('std::unique_ptr<lwt::generic::FastGraph<double> >','std::unique_ptr<CachedNNGraph>');p.write_text(s)
p=dest/'Root/JetVertexNNTagger.cxx';s=p.read_text().replace('#include "lwtnn/generic/FastGraph.hh"','#include "nn_graph.hxx"').replace('std::make_unique<lwt::generic::FastGraph<double> >','std::make_unique<CachedNNGraph>');p.write_text(s)
p=dest/'CMakeLists.txt';p.write_text(p.read_text()+'\n# Shared inference backend; B configuration remains unchanged.\nfind_package(Eigen3 REQUIRED)\ntarget_include_directories(JetMomentToolsLib PRIVATE "/home/gwatts/atlas/correctionlib_tests/src" ${LWTNN_INCLUDE_DIRS})\ntarget_link_libraries(JetMomentToolsLib PRIVATE Eigen3::Eigen ${LWTNN_LIBRARIES})\n')
