#pragma once
#include <nlohmann/json.hpp>
#include <map>
class JetSystematics {
 using Json=nlohmann::json;
 std::unique_ptr<correction::CorrectionSet> tables;
 Json manifest;

 struct CompiledComponent {
  std::string name,hist,mode;bool absEta,special,resolution;
  CompiledComponent(const Json& c):name(c.at("Name")),hist(c.value("Hists",name)),
   mode(c.value("Interp",std::string("Full"))),absEta(c.at("Param")=="PtAbsEta"),
   special(c.value("Special",std::string())=="True"),
   resolution(c.value("Scale",std::string())=="FourVecResAbs"){}
 };
 double muRef,npvRef;
 std::map<std::string,std::vector<CompiledComponent>> recipes;
 double table(const std::string& name,double pt,double eta,const std::string& mode="Full",int file=0)const{
  auto c=tables->at("file"+std::to_string(file)+"__"+name+"_AntiKt4EMPFlow__"+mode+"__content");
  if(c->inputs().size()==1)return c->evaluate({pt});
  return c->evaluate({pt,eta});
 }
 double component(const CompiledComponent& c,double pt,double eta,double mu,double npv,bool bjet)const{
  const std::string &name=c.name,&hist=c.hist,&mode=c.mode;
  double y=c.absEta?std::abs(eta):eta;
  auto h=[&](const std::string& key){return table(key,pt,y,mode);};
  if(!c.special)return h(hist);
  double dm=mu-muRef;
  double dn=npv-npvRef;
  if(name=="Pileup_OffsetMu_orig")return dm*h(hist);
  if(name=="Pileup_OffsetNPV_orig")return dn*h(hist);
  if(name=="Pileup_PtTerm_orig"){auto split=hist.find(',');return dm*h(hist.substr(0,split))+dn*h(hist.substr(split+1));}
  if(name=="Pileup_RhoTopology_orig")return h(hist);
  if(name=="BJES_Response")return bjet?h(hist):0.;
  if(bjet)return 0.;
  double fg=table("gluonFraction",pt,std::abs(eta),"Full",1);
  if(name=="Flavor_Response_orig")return h(hist)*fg;
  if(name=="Flavor_Composition_orig"){
   auto split=hist.find(',');double rg=h(hist.substr(0,split)),rq=h(hist.substr(split+1));
   double df=table("gluonFractionError",pt,std::abs(eta),"Full",1);
   return df*std::abs(rq-rg)/(fg*rg+(1-fg)*rq);
  }
  throw std::invalid_argument("Unsupported special uncertainty component: "+name);
 }
public:
 JetSystematics(const std::string& payload,const std::string& config):tables(correction::CorrectionSet::from_file(payload)){
  std::ifstream input(config);if(!input)throw std::runtime_error("Missing uncertainty manifest");input>>manifest;
  muRef=std::stod(manifest.at("settings").at("Pileup.MuRef").get<std::string>());
  npvRef=std::stod(manifest.at("settings").at("Pileup.NPVRef").get<std::string>());
  for(const auto& c:manifest.at("components"))if(!c.contains("Group"))recipes[c.at("Name")].push_back(c);
  for(auto it=manifest.at("groups").begin();it!=manifest.at("groups").end();++it){
   const auto& g=it.value();if(g.at("Corr")!="Correlated")throw std::invalid_argument("Unsupported group correlation");
   for(const auto& c:manifest.at("components"))if(c.value("Group",std::string())==it.key())recipes[g.at("Name")].push_back(c);
  }
 }
 std::vector<std::string> names()const{
  std::vector<std::string> out{"NOSYS"};
  for(const auto& [name,cs]:recipes){
   bool jer=cs.front().resolution;
   for(int data=0;data<=(jer?1:0);++data)for(const auto& sign:{"up","down"})out.push_back("JET_"+name+(data?"_PseudoData":"")+"__1"+sign);
  }
  return out;
 }
 // pt,eta,phi,m,mu,npv,isBjet. Status 0 is CorrectionCode::Ok.
 std::vector<double> evaluate(const std::string& sys,const std::vector<double>& r)const{
  if(r.size()!=7)throw std::invalid_argument("Systematics requires seven inputs");
  if(sys=="NOSYS")return {r[0],r[1],r[2],r[3],0.};
  if(sys.rfind("JET_",0)!=0)throw std::invalid_argument("Invalid systematic name");
  auto split=sys.find("__1");if(split==std::string::npos)throw std::invalid_argument("Invalid systematic direction");
  std::string name=sys.substr(4,split-4),direction=sys.substr(split+3);
  if(direction!="up"&&direction!="down")throw std::invalid_argument("Unsupported systematic direction");
  double sign=direction=="up"?1.:-1.;bool data=false;
  const std::string suffix="_PseudoData";
  if(name.size()>suffix.size()&&name.substr(name.size()-suffix.size())==suffix){data=true;name.resize(name.size()-suffix.size());}
  const auto& cs=recipes.at(name);
  const double pt=r[0]*double(float(.001)),eta=r[1];
  double unc=0;for(const auto& c:cs)unc+=component(c,pt,eta,r[4],r[5],r[6]!=0);
  double scale=1+sign*unc;
  if(cs.front().resolution){
   if(unc==0 || (data && sign*unc>0) || (!data && sign*unc<0))scale=1.;
   else {
    double sigma=std::max(table("JER_Nominal_MC16",pt,std::abs(eta),"OnlyX"),table("JER_Nominal_data",pt,std::abs(eta),"OnlyX"));
    scale=smear(r[2],sigma,sign*unc,data);
   }
  }else if(data)throw std::invalid_argument("PseudoData is only supported for resolution variations");
  return {double(float(r[0]*scale)),r[1],r[2],double(float(r[3]*scale)),0.};
 }
 std::vector<std::vector<double>> batch(const std::string& sys,const std::vector<std::vector<double>>& rows)const{
  std::vector<std::vector<double>> out;out.reserve(rows.size());for(const auto& r:rows)out.push_back(evaluate(sys,r));return out;
 }
};
