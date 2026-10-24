#pragma once
#include <lwtnn/generic/Stack.hh>
#include <lwtnn/lightweight_network_config.hh>
#include <array>
#include <algorithm>
#include <cstdint>

// A conservative dense-chain execution plan. Same dynamic Eigen products,
// separate bias/activation operations, and preprocessing as lwtnn; no fusion.
// Unsupported graphs continue through the original FastGraph implementation.
class BufferedNN {
 using Vector=lwt::VectorX<double>;
 using Matrix=lwt::MatrixX<double>;
 enum class Kind {Matrix,Bias,Relu,Sigmoid};
 struct Operation {Kind kind;Matrix matrix;Vector bias;size_t outputs;};
 std::vector<Operation> operations;
 Vector offsets,scales;
 std::array<size_t,4> indices{};
 bool ready=false;
 struct Workspace {uint64_t owner=0;Vector raw;std::vector<Vector> values;};
public:
 BufferedNN(const lwt::GraphConfig& cfg,const lwt::InputOrder& order){
  if(cfg.inputs.size()!=1||cfg.inputs[0].variables.size()!=4||!cfg.input_sequences.empty()||cfg.outputs.size()!=1||order.scalar.size()!=1||order.scalar[0].second.size()!=4||!order.sequence.empty())return;
  if(cfg.nodes.empty()||cfg.outputs.begin()->second.node_index!=cfg.nodes.size()-1||cfg.outputs.begin()->second.labels.size()!=1)return;
  const auto& first=cfg.nodes[0];
  if(first.type!=lwt::NodeConfig::Type::INPUT||first.index!=4||first.sources!=std::vector<size_t>{0})return;
  offsets.resize(4);scales.resize(4);
  for(size_t i=0;i<4;++i){const auto& variable=cfg.inputs[0].variables[i];const auto& names=order.scalar[0].second;
   auto found=std::find(names.begin(),names.end(),variable.name);if(found==names.end())return;indices[i]=found-names.begin();offsets(i)=variable.offset;scales(i)=variable.scale;}
  size_t width=4;
  for(size_t i=1;i<cfg.nodes.size();++i){
   const auto& node=cfg.nodes[i];
   if(node.type!=lwt::NodeConfig::Type::FEED_FORWARD||node.sources!=std::vector<size_t>{i-1}||node.index<0||size_t(node.index)>=cfg.layers.size())return;
   const auto& layer=cfg.layers[node.index];
   if(layer.architecture!=lwt::Architecture::DENSE||!layer.U.empty()||!layer.sublayers.empty()||!layer.components.empty()||!layer.embedding.empty())return;
   if(layer.activation.function!=lwt::Activation::LINEAR&&layer.activation.function!=lwt::Activation::RECTIFIED&&layer.activation.function!=lwt::Activation::SIGMOID)return;
   if(!layer.weights.empty()){
    if(!width||layer.weights.size()%width)return;
    auto matrix=lwt::generic::build_matrix<double>(layer.weights,width);width=matrix.rows();
    operations.push_back({Kind::Matrix,std::move(matrix),{},width});
   }
   if(!layer.bias.empty()){
    if(layer.bias.size()!=width)return;
    operations.push_back({Kind::Bias,{},lwt::generic::build_vector<double>(layer.bias),width});
   }
   if(layer.activation.function==lwt::Activation::RECTIFIED)operations.push_back({Kind::Relu,{},{},width});
   if(layer.activation.function==lwt::Activation::SIGMOID)operations.push_back({Kind::Sigmoid,{},{},width});
  }
  ready=width==1;
 }
 bool supported()const{return ready;}
 double evaluate(const std::array<double,4>& input,uint64_t id)const{
  static thread_local Workspace workspace;
  if(workspace.owner!=id){
   workspace.raw.resize(4);workspace.values.clear();workspace.values.reserve(operations.size()+1);workspace.values.emplace_back(4);
   for(const auto& operation:operations)workspace.values.emplace_back(operation.outputs);
   workspace.owner=id;
  }
  for(size_t i=0;i<4;++i)workspace.raw(i)=input[indices[i]];
  workspace.values[0]=(workspace.raw+offsets).cwiseProduct(scales);
  for(size_t i=0;i<operations.size();++i){
   const auto& operation=operations[i];const auto& in=workspace.values[i];auto& out=workspace.values[i+1];
   switch(operation.kind){
    case Kind::Matrix:out.noalias()=operation.matrix*in;break;
    case Kind::Bias:out=in+operation.bias;break;
    case Kind::Relu:out=in.unaryExpr([](double x){return x<0?0:x;});break;
    case Kind::Sigmoid:out=in.unaryExpr([](double x){if(x<-30.)return 0.;if(x>30.)return 1.;return 1./(1.+std::exp(-1.*x));});break;
   }
  }
  return workspace.values.back()(0);
 }
};
