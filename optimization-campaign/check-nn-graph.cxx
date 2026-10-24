#include <nn_graph.hxx>
#include <lwtnn/parse_json.hh>
#include <fstream>
#include <iostream>
#include <random>
#include <thread>
#include <atomic>
int main(){
 std::ifstream f("payloads/nnjvt-network.json");auto cfg=lwt::parse_json_graph(f);lwt::InputOrder order;order.scalar.emplace_back(cfg.inputs[0].name,std::vector<std::string>{"Rpt","JVFCorr","ptbin","etabin"});
 CachedNNGraph engine(cfg,order);std::mt19937 rng(1234);std::uniform_real_distribution<double> d(-2,3);std::vector<std::array<double,4>> inputs;
 for(int i=0;i<100000;++i)inputs.push_back({double(float(d(rng))),double(float(d(rng))),double(i%4),double(i%5)});
 std::atomic<size_t> count{0};
 auto check=[&](const std::array<double,4>& x){double a=engine.score4(x),b=engine.reference_score4(x);if(std::memcmp(&a,&b,8))throw std::runtime_error("NN score differs");++count;};
 for(auto& x:inputs)check(x);for(auto& x:inputs)check(x);for(auto i=inputs.rbegin();i!=inputs.rend();++i)check(*i);
 for(double x:{-0.,0.,-1.e10,1.e10})check({x,x,0,0});
 std::thread a([&]{for(size_t i=0;i<1000;++i)check(inputs[i]);}),b([&]{for(size_t i=1000;i<2000;++i)check(inputs[i]);});a.join();b.join();
 auto otherCfg=cfg;otherCfg.layers[0].bias[0]+=1;CachedNNGraph other(otherCfg,order);for(size_t i=0;i<100;++i){auto x=inputs[i];auto u=other.score4(x),v=other.reference_score4(x);if(std::memcmp(&u,&v,8))return 4;}
 for(size_t i=0;i<100;++i)check(inputs[i]);
#ifdef EIGEN_RUNTIME_NO_MALLOC
 if(!BufferedNN(cfg,order).supported())throw std::runtime_error("Buffer path was not enabled");
 engine.uncached_score4(inputs[0]);
 Eigen::internal::set_is_malloc_allowed(false);
 for(size_t i=0;i<1000;++i)engine.uncached_score4(inputs[i]);
 Eigen::internal::set_is_malloc_allowed(true);
 auto fallbackCfg=cfg;fallbackCfg.layers.back().activation.function=lwt::Activation::TANH;
 if(BufferedNN(fallbackCfg,order).supported())throw std::runtime_error("Unsupported activation accepted");
 CachedNNGraph fallback(fallbackCfg,order);
 for(size_t i=0;i<1000;++i){auto u=fallback.score4(inputs[i]),v=fallback.reference_score4(inputs[i]);if(std::memcmp(&u,&v,8))return 5;}
 auto reordered=order;std::swap(reordered.scalar[0].second[0],reordered.scalar[0].second[1]);CachedNNGraph permuted(cfg,reordered);
 for(size_t i=0;i<1000;++i){auto u=permuted.score4(inputs[i]),v=permuted.reference_score4(inputs[i]);if(std::memcmp(&u,&v,8))return 6;}
 std::cout<<"PASS buffered path active; no Eigen allocations after warmup; unsupported-activation fallback and permuted input order\n";
#endif
 std::cout<<"PASS"<<count<<" exact comparisons: repeats, collisions, reverse traversal, thread isolation, model replacement, extreme/signed-zero inputs\n";
}


