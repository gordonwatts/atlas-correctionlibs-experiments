#include <algorithm>
#include "lookup.hxx"
#include "backend.hxx"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <sched.h>

using Clock=std::chrono::steady_clock;
using Json=nlohmann::json;
volatile double sink=0;
struct Check {
 size_t count=0,bad=0,unequal=0;double maxAbs=0,maxRel=0;
 void add(double a,double b,double atol=1e-10,double rtol=1e-9){
  ++count;unequal+=a!=b;double d=std::abs(a-b);maxAbs=std::max(maxAbs,d);
  maxRel=std::max(maxRel,d/std::max(1e-30,std::abs(a)));
  bad+=!std::isfinite(d)||d>atol+rtol*std::abs(a);
 }
 Json json()const{return Json{{"count",count},{"bad",bad},{"unequal",unequal},{"max_abs",maxAbs},{"max_rel",maxRel}};}
};
template<class F> double timed(F f){auto t=Clock::now();sink=f();return std::chrono::duration<double>(Clock::now()-t).count();}
int main(int argc,char**argv){
 if(argc!=3)return 2;std::string base=argv[1],out=argv[2];
 cpu_set_t allowed;CPU_ZERO(&allowed);sched_getaffinity(0,sizeof(allowed),&allowed);
 int cpu=0;while(!CPU_ISSET(cpu,&allowed))++cpu;cpu_set_t single;CPU_ZERO(&single);CPU_SET(cpu,&single);sched_setaffinity(0,sizeof(single),&single);
 std::ifstream(base+"/payloads/uncertainty-manifest.json")>>lookupManifest;
 JetSystematics evaluator(base+"/payloads/uncertainty-tables.json.gz",base+"/payloads/uncertainty-manifest.json");
 std::ifstream input(base+"/results/overhead-study/kernel-inputs.bin",std::ios::binary);
 uint64_t n;input.read(reinterpret_cast<char*>(&n),8);std::vector<double> original(n*21),rows(n*7);
 input.read(reinterpret_cast<char*>(original.data()),original.size()*8);if(!input)throw std::runtime_error("Bad replay input");
 for(size_t i=0;i<n;++i)std::copy_n(original.data()+21*i+14,7,rows.data()+7*i);
 Json report={{"jets",n},{"cpu",cpu},{"lookup_tolerance",{{"atol",1e-10},{"rtol",1e-9}}}};
 std::map<std::string,std::vector<Lookup*>> categories;
 for(auto& [name,p]:lookups)categories[(p->one?"1D-":"2D-")+p->mode].push_back(p.get());
 Check all;bool failed=false;
 for(auto& [category,tables]:categories){
  Check check,edges;
  for(auto* table:tables){
   for(size_t i=0;i<n;++i){double pt=rows[7*i]*double(float(.001)),eta=std::abs(rows[7*i+1]);check.add(table->root(pt,eta),table->correction(pt,eta));}
   auto* hist=table->hist->getHisto();const TAxis* axes[]={hist->GetXaxis(),hist->GetYaxis()};
   for(int dim=0;dim<(table->one?1:2);++dim){auto* axis=axes[dim];
    for(int b=1;b<=axis->GetNbins()+1;++b)for(double x:{axis->GetBinLowEdge(b),std::nextafter(axis->GetBinLowEdge(b),-INFINITY),std::nextafter(axis->GetBinLowEdge(b),INFINITY)}){
     double pt=hist->GetXaxis()->GetBinCenter(std::max(1,hist->GetNbinsX()/2));double eta=hist->GetYaxis()->GetBinCenter(std::max(1,hist->GetNbinsY()/2));
     if(dim==0)pt=x;else eta=x;edges.add(table->root(pt,eta),table->correction(pt,eta));
    }
   }
  }
  Json item={{"tables",tables.size()},{"lookups_per_sweep",n*tables.size()},{"sample_validation",check.json()},{"boundary_validation",edges.json()}};
  failed|=check.bad||edges.bad;
  for(int rep=0;rep<7;++rep){
   Json round;
   for(int k=0;k<3;++k){int mode=(k+rep)%3;
    double seconds=timed([&]{double sum=0;for(auto* table:tables){std::vector<correction::Variable::Type> args(table->one?1:2,0.);
     for(size_t i=0;i<n;++i){double pt=rows[7*i]*double(float(.001)),eta=std::abs(rows[7*i+1]);
      sum+=mode==0?table->root(pt,eta):mode==1?table->correction(pt,eta):table->reused(pt,eta,args);
     }}return sum;});
    round[mode==0?"atlas":mode==1?"correctionlib":"correctionlib_reused_arguments"]=seconds;
   }item["rounds"].push_back(round);
  }report["lookup_categories"][category]=item;
  std::cout<<category<<" "<<item.dump()<<std::endl;
 }
 auto groups=evaluator.groups();size_t maxGroup=0,states=0;
 for(auto& group:groups){maxGroup=std::max(maxGroup,group.size());states+=group.size();}
 std::vector<double> a(n*maxGroup*5),b(a.size());Check full;
 for(auto& group:groups){
  useRootLookup=true;evaluator.batch_group_into(group,rows.data(),n,a.data());
  useRootLookup=false;evaluator.batch_group_into(group,rows.data(),n,b.data());
  for(size_t i=0;i<n*group.size()*5;++i){size_t col=i%5;
   full.add(a[i],b[i],col==0||col==3?.01:col==4?0:2e-6,col==0||col==3?2e-6:0);
  }
 }
 failed|=full.bad;report["shared_systematics"]={{"groups",groups.size()},{"states",states},{"validation",full.json()}};
 for(int rep=0;rep<7;++rep){Json round;
  for(int k=0;k<2;++k){bool old=(k+rep)%2==0;useRootLookup=old;
   double seconds=timed([&]{double sum=0;for(auto& group:groups){evaluator.batch_group_into(group,rows.data(),n,a.data());sum+=a[(group.size()*n-1)*5];}return sum;});
   round[old?"atlas":"correctionlib"]=seconds;
  }report["shared_systematics"]["rounds"].push_back(round);std::cout<<"grouped "<<round.dump()<<std::endl;
 }
 report["passed"]=!failed;
 std::ifstream maps("/proc/self/maps");std::ofstream saved(out+"/loaded-libraries.txt");saved<<maps.rdbuf();
 std::ofstream(out+"/raw.json")<<report.dump(2)<<"\n";
 return failed?1:0;
}
