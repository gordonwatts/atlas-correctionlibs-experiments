#pragma once
#include <lwtnn/generic/FastGraph.hh>
#include <lwtnn/Stack.hh>
#include "nn_buffered.hxx"
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

// Shared by the columnar evaluator and the unchanged-config ATLAS NN tagger.
// The cache stores scores only; callers still apply current selection/validity.
class CachedNNGraph {
 using Key=std::array<uint64_t,4>;
 struct Entry {Key key{};double score=0;bool valid=false;};
 struct Cache {uint64_t owner=0;std::vector<Entry> entries;};
 inline static std::atomic<uint64_t> nextId{1};
 const uint64_t id=nextId.fetch_add(1);
 lwt::generic::FastGraph<double> graph;
 BufferedNN buffered;
 bool supported;
 bool cacheEnabled;
 static Key key(const std::array<double,4>& input){Key out;std::memcpy(out.data(),input.data(),sizeof(out));return out;}
 static size_t slot(const Key& k){
  uint64_t h=0x9e3779b97f4a7c15ULL;
  for(auto v:k){v^=v>>30;v*=0xbf58476d1ce4e5b9ULL;v^=v>>27;v*=0x94d049bb133111ebULL;v^=v>>31;h^=v+0x9e3779b97f4a7c15ULL+(h<<6)+(h>>2);}
  return h&((1u<<18)-1);
 }
public:
 CachedNNGraph(const lwt::GraphConfig& cfg,const lwt::InputOrder& order,bool enabled=true):graph(cfg,order),buffered(cfg,order),
  supported(cfg.inputs.size()==1&&cfg.inputs[0].variables.size()==4&&cfg.input_sequences.empty()&&cfg.outputs.size()==1&&cfg.outputs.begin()->second.labels.size()==1),cacheEnabled(enabled){}
 double reference_score4(const std::array<double,4>& input)const{
  std::vector<double> f(input.begin(),input.end());std::vector<lwt::VectorX<double>> nodes{lwt::build_vector(f)};
  return graph.compute(nodes)(0);
 }
 double uncached_score4(const std::array<double,4>& input)const{
  return buffered.supported()?buffered.evaluate(input,id):reference_score4(input);
 }
 double score4(const std::array<double,4>& input)const{
  if(!supported)return reference_score4(input);
  if(!cacheEnabled)return uncached_score4(input);
  for(double x:input)if(!std::isfinite(x))return reference_score4(input);
  static thread_local Cache cache;
  if(cache.owner!=id){cache.entries.assign(1u<<18,Entry{});cache.owner=id;}
  const auto k=key(input);auto& entry=cache.entries[slot(k)];
  if(entry.valid&&entry.key==k)return entry.score;
  const double value=uncached_score4(input);
  // Retain the first feature tuple in each slot, favoring nominal inputs.
  // Collisions always evaluate normally; no hash-only equality is allowed.
  if(!entry.valid)entry={k,value,true};
  return value;
 }
 lwt::VectorX<double> compute(const std::vector<lwt::VectorX<double>>& nodes)const{
  if(!supported||nodes.size()!=1||nodes[0].size()!=4)return graph.compute(nodes);
  lwt::VectorX<double> result(1);result(0)=score4({nodes[0](0),nodes[0](1),nodes[0](2),nodes[0](3)});return result;
 }
};
