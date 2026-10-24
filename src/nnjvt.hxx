#include <array>
#pragma once
#include "nn_graph.hxx"
#include <lwtnn/parse_json.hh>
#include <lwtnn/InputOrder.hh>
#include <lwtnn/Stack.hh>
#include <fstream>
#include <algorithm>
class NNJvt {
 std::unique_ptr<CachedNNGraph> graph;
 std::vector<float> ptEdges,etaEdges;
 std::vector<std::vector<float>> cuts;
 bool bins(float pt,float eta,size_t& ip,size_t& ie) const {
  ip=std::lower_bound(ptEdges.begin(),ptEdges.end(),pt)-ptEdges.begin();
  ie=std::lower_bound(etaEdges.begin(),etaEdges.end(),eta)-etaEdges.begin();
  if(ip==0)ip=1;
  if(ip==ptEdges.size()||ie==0||ie==etaEdges.size())return false;
  --ip;--ie;return true;
 }
public:
 NNJvt(const std::string& path,std::vector<float> pe,std::vector<float> ee,std::vector<std::vector<float>> c):ptEdges(pe),etaEdges(ee),cuts(c){
  std::ifstream input(path);if(!input)throw std::runtime_error("Cannot open NNJVT network");
  auto cfg=lwt::parse_json_graph(input);
  lwt::InputOrder order;order.scalar.emplace_back(cfg.inputs.at(0).name,std::vector<std::string>{"Rpt","JVFCorr","ptbin","etabin"});
  graph=std::make_unique<CachedNNGraph>(cfg,order);
 }
 // pt, eta, DetectorEta, sumPtTrkPV, JVFCorr, hasPV
 std::array<double,2> evaluate_row(const std::vector<double>& r) const {
   if(r.size()!=6)throw std::invalid_argument("NNJVT requires six inputs");
   float score=-1;size_t ip=0,ie=0;
   if(r[5]!=0 && r[0]<=60000 && bins(float(r[0]),float(r[1]),ip,ie)){
    float rpt=r[0]!=0?float(r[3]/r[0]):0;
    score=graph->score4({rpt,float(r[4]),float(ip),float(ie)});
   }
   float cut=-1;
   if(bins(float(r[0]),float(r[2]),ip,ie))cut=cuts.at(ip).at(ie);
   bool inRange=r[0]>=20000&&r[0]<=60000&&std::abs(float(r[2]))<=2.5;
   return {score,(!inRange||score>cut)?1.:0.};
 }
 std::vector<std::vector<double>> batch(const std::vector<std::vector<double>>& rows) const {
  std::vector<std::vector<double>> out;out.reserve(rows.size());
  for(const auto& r:rows){const auto result=evaluate_row(r);out.emplace_back(result.begin(),result.end());}
  return out;
 }
};
