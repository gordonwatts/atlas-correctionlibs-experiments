#include <TFile.h>
#include <TH2.h>
#include <correction.h>
#include <nlohmann/json.hpp>
#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <sched.h>
#include <vector>
using Json=nlohmann::json;
volatile double jvtSink=0;
// Lookup sequence from AnalysisBase 25.2.100 JvtEfficiencyToolBase.cxx.
// Original ATLAS implementation copyright (C) 2002-2024 CERN.
std::array<float,2> rootLookup(const TH2& h,float x,float y){
 int ix=h.GetXaxis()->FindBin(x);
 if(ix==0||ix==h.GetNbinsX()+1)return {-1,-1};
 int iy=h.GetYaxis()->FindBin(y);
 if(iy==0||iy==h.GetNbinsY()+1)return {-1,-1};
 return {float(h.GetBinContent(ix,iy)),float(h.GetBinError(ix,iy))};
}
int main(int argc,char**argv){
 if(argc!=3)return 2;std::string base=argv[1],out=argv[2];
 cpu_set_t mask;CPU_ZERO(&mask);sched_getaffinity(0,sizeof(mask),&mask);int cpu=0;while(!CPU_ISSET(cpu,&mask))++cpu;CPU_ZERO(&mask);CPU_SET(cpu,&mask);sched_setaffinity(0,sizeof(mask),&mask);
 Json manifest;std::ifstream(base+"/payloads/uncertainty-manifest.json")>>manifest;
 std::string file;for(auto it=manifest["sources"].begin();it!=manifest["sources"].end();++it)if(it.key().find("NNJvtSFFile")!=std::string::npos)file=it.key();
 std::unique_ptr<TFile> f(TFile::Open(file.c_str()));auto cs=correction::CorrectionSet::from_file(base+"/payloads/uncertainty-tables.json.gz");
 std::ifstream input(base+"/results/overhead-study/kernel-inputs.bin",std::ios::binary);uint64_t n;input.read((char*)&n,8);std::vector<double> rows(n*21);input.read((char*)rows.data(),rows.size()*8);if(!input)return 2;
 Json result;bool failed=false;
 for(const auto& name:{"JvtFixedEffPt","EffFixedEffPt"}){
  auto* h=dynamic_cast<TH2*>(f->Get(name));if(!h)return 3;
  std::string prefix=std::string("file2__")+name+"__None__";
  auto value=cs->at(prefix+"content"),error=cs->at(prefix+"error");
  std::vector<std::array<float,2>> points;
  for(size_t i=0;i<n;++i){float x=rows[21*i+14],y=rows[21*i+15];
   if(x>=h->GetXaxis()->GetXmin()&&x<h->GetXaxis()->GetXmax()&&y>=h->GetYaxis()->GetXmin()&&y<h->GetYaxis()->GetXmax())points.push_back({x,y});
  }
  size_t bad=0;double maxAbs=0;for(auto p:points){auto a=rootLookup(*h,p[0],p[1]);std::array<float,2>b={float(value->evaluate({double(p[0]),double(p[1])})),float(error->evaluate({double(p[0]),double(p[1])}))};
   for(int k=0;k<2;++k){double d=std::abs(double(a[k])-b[k]);maxAbs=std::max(maxAbs,d);bad+=a[k]!=b[k]||!std::isfinite(d);}
  }
  failed|=bad;Json item={{"points",points.size()},{"pairs_per_sweep",points.size()*64},{"mismatches",bad},{"max_abs",maxAbs}};
  std::vector<correction::Variable::Type> args{0.,0.};
  for(int rep=0;rep<7;++rep){Json round;for(int k=0;k<3;++k){int mode=(rep+k)%3;double sum=0;auto start=std::chrono::steady_clock::now();
   for(int repeat=0;repeat<64;++repeat)for(auto p:points){std::array<float,2> r;
    if(mode==0)r=rootLookup(*h,p[0],p[1]);
    else if(mode==1)r={float(value->evaluate({double(p[0]),double(p[1])})),float(error->evaluate({double(p[0]),double(p[1])}))};
    else{args[0]=double(p[0]);args[1]=double(p[1]);r={float(value->evaluate(args)),float(error->evaluate(args))};}
    sum+=r[0]+double(r[1]);
   }
   jvtSink=sum;round[mode==0?"atlas":mode==1?"correctionlib":"correctionlib_reused_arguments"]=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
  }item["rounds"].push_back(round);}
  result[name]=item;std::cout<<name<<" "<<item.dump()<<std::endl;
 }
 result["passed"]=!failed;result["cpu"]=cpu;std::ofstream(out+"/jvt.json")<<result.dump(2)<<"\n";return failed?1:0;
}
