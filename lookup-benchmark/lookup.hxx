#pragma once
#include <JetUncertainties/UncertaintyHistogram.h>
#include <TFile.h>
#include <TH2.h>
#include <correction.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <map>
#include <memory>
#include <string>

inline bool useRootLookup=false;
struct Lookup {
 std::string name,mode;
 bool one;
 correction::Correction::Ref corr;
 std::unique_ptr<jet::UncertaintyHistogram> hist;
 double root(double pt,double eta)const {return one?hist->getValue(pt):hist->getValue(pt,eta);}
 double correction(double pt,double eta)const {return one?corr->evaluate({pt}):corr->evaluate({pt,eta});}
 double reused(double pt,double eta,std::vector<correction::Variable::Type>& args)const {
  args[0]=pt;if(!one)args[1]=eta;return corr->evaluate(args);
 }
};
inline std::map<std::string,std::unique_ptr<Lookup>> lookups;
inline nlohmann::json lookupManifest;
inline Lookup* bindLookup(const std::string& name,correction::Correction::Ref corr){
 if(lookups.count(name))return lookups.at(name).get();
 const auto& spec=lookupManifest.at("mapping").at(name);
 std::string source;
 for(auto it=lookupManifest.at("sources").begin();it!=lookupManifest.at("sources").end();++it)
  if(it.value().at("logical_path")==spec.at("file"))source=it.key();
 if(source.empty())throw std::runtime_error("Missing ROOT source: "+name);
 auto p=std::make_unique<Lookup>();p->name=name;p->corr=corr;p->one=corr->inputs().size()==1;
 p->mode=spec.at("mode");
 auto mode=p->mode=="Full"?jet::Interpolate::Full:jet::Interpolate::OnlyX;
 p->hist=std::make_unique<jet::UncertaintyHistogram>(spec.at("object").get<std::string>(),mode);
 std::unique_ptr<TFile> file(TFile::Open(source.c_str()));
 if(!file||file->IsZombie()||p->hist->initialize(file.get()).isFailure())throw std::runtime_error("Cannot load "+name);
 auto result=p.get();lookups.emplace(name,std::move(p));return result;
}
