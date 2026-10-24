#include "backend.hxx"
#include "nnjvt.hxx"
#include <valgrind/callgrind.h>
#include <iostream>
#include <cstdint>
struct Batch {int64_t n,width,sigma;std::vector<double> values;};
int main(int argc,char**argv){
 if(argc!=3)return 2;std::string mode=argv[1];std::ifstream in(argv[2],std::ios::binary);std::vector<Batch> batches;
 for(Batch b;in.read(reinterpret_cast<char*>(&b.n),8);){in.read(reinterpret_cast<char*>(&b.width),8);in.read(reinterpret_cast<char*>(&b.sigma),8);b.values.resize(b.n*b.width);in.read(reinterpret_cast<char*>(b.values.data()),b.values.size()*8);if(!in)return 3;batches.push_back(std::move(b));}
 double checksum=0;
 if(mode=="nn"){
  std::ifstream f("payloads/nnjvt-cuts.json");nlohmann::json c;f>>c;
  auto pt=c["ptbin_edges"].get<std::vector<float>>(),eta=c["etabin_edges"].get<std::vector<float>>();for(auto& x:pt)x*=float(1000);
  std::vector<std::vector<float>> cuts(pt.size()-1,std::vector<float>(eta.size()-1));
  for(size_t i=0;i<cuts.size();++i)for(size_t j=0;j<cuts[i].size();++j)cuts[i][j]=c["cuts"]["("+std::to_string(i)+", "+std::to_string(j)+")"].get<float>();
  NNJvt engine("payloads/nnjvt-network.json",pt,eta,cuts);std::vector<double> row(6);
  CALLGRIND_START_INSTRUMENTATION;
  for(const auto& b:batches)for(int64_t i=0;i<b.n;++i){std::copy_n(b.values.data()+i*6,6,row.begin());auto v=engine.evaluate_row(row);checksum+=v[0]+v[1];}
  CALLGRIND_STOP_INSTRUMENTATION;
 }else if(mode=="efficiency"){
  JvtEfficiency engine("payloads/uncertainty-tables.json.gz","payloads/uncertainty-manifest.json");std::vector<double> row(4);
  CALLGRIND_START_INSTRUMENTATION;
  for(const auto& b:batches)for(int64_t i=0;i<b.n;++i){std::copy_n(b.values.data()+i*4,4,row.begin());auto v=engine.evaluate_row(row,b.sigma);checksum+=v[0]+v[1];}
  CALLGRIND_STOP_INSTRUMENTATION;
 }else{
  JetSystematics engine("payloads/uncertainty-tables.json.gz","payloads/uncertainty-manifest.json");auto groups=engine.groups();const auto& b=batches.at(0);std::vector<double> out(b.n*5*4);
  CALLGRIND_START_INSTRUMENTATION;
  for(const auto& group:groups){engine.batch_group_into(group,b.values.data(),b.n,out.data());checksum+=out[0];}
  CALLGRIND_STOP_INSTRUMENTATION;
 }
 CALLGRIND_DUMP_STATS;std::cout<<mode<<" batches="<<batches.size()<<" checksum="<<checksum<<"\n";
}
