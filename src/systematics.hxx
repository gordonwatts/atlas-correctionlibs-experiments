#pragma once
#include <nlohmann/json.hpp>
#include <map>
class JetSystematics {
 using Json=nlohmann::json;
 std::unique_ptr<correction::CorrectionSet> tables;
 Json manifest;

 struct Table {
  correction::Correction::Ref correction;
  bool oneInput=false;
  double operator()(double pt,double eta)const{
   return oneInput?correction->evaluate({pt}):correction->evaluate({pt,eta});
  }
 };
 Table bind(const std::string& name,const std::string& mode="Full",int file=0)const{
  auto c=tables->at("file"+std::to_string(file)+"__"+name+"_AntiKt4EMPFlow__"+mode+"__content");
  return {c,c->inputs().size()==1};
 }
 Table gluonFraction,gluonFractionError,jerMC,jerData;
 struct CompiledComponent {
  Table first,second;
  std::string name,hist,mode;bool absEta,special,resolution;
  CompiledComponent(const Json& c):name(c.at("Name")),hist(c.value("Hists",name)),
   mode(c.value("Interp",std::string("Full"))),absEta(c.at("Param")=="PtAbsEta"),
   special(c.value("Special",std::string())=="True"),
   resolution(c.value("Scale",std::string())=="FourVecResAbs"){}
 };
 double muRef,npvRef;
 std::map<std::string,std::vector<CompiledComponent>> recipes;
 double component(const CompiledComponent& c,double pt,double eta,double mu,double npv,bool bjet)const{
  const std::string &name=c.name,&hist=c.hist,&mode=c.mode;
  double y=c.absEta?std::abs(eta):eta;

  if(!c.special)return c.first(pt,y);
  double dm=mu-muRef;
  double dn=npv-npvRef;
  if(name=="Pileup_OffsetMu_orig")return dm*c.first(pt,y);
  if(name=="Pileup_OffsetNPV_orig")return dn*c.first(pt,y);
  if(name=="Pileup_PtTerm_orig"){return dm*c.first(pt,y)+dn*c.second(pt,y);}
  if(name=="Pileup_RhoTopology_orig")return c.first(pt,y);
  if(name=="BJES_Response")return bjet?c.first(pt,y):0.;
  if(bjet)return 0.;
  double fg=gluonFraction(pt,std::abs(eta));
  if(name=="Flavor_Response_orig")return c.first(pt,y)*fg;
  if(name=="Flavor_Composition_orig"){
   double rg=c.first(pt,y),rq=c.second(pt,y);
   double df=gluonFractionError(pt,std::abs(eta));
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
  // Bind immutable correction objects once. Per-jet evaluation retains the
  // same values, dimensions and arithmetic order without constructing keys.
  for(auto& [name,cs]:recipes)for(auto& c:cs){
   if(c.special && (c.name=="Pileup_PtTerm_orig" || c.name=="Flavor_Composition_orig")){
    auto split=c.hist.find(',');
    c.first=bind(c.hist.substr(0,split),c.mode);
    c.second=bind(c.hist.substr(split+1),c.mode);
   }else c.first=bind(c.hist,c.mode);
   if(c.special && (c.name=="Flavor_Response_orig" || c.name=="Flavor_Composition_orig"))
    gluonFraction=bind("gluonFraction","Full",1);
   if(c.special && c.name=="Flavor_Composition_orig")gluonFractionError=bind("gluonFractionError","Full",1);
   if(c.resolution){jerMC=bind("JER_Nominal_MC16","OnlyX");jerData=bind("JER_Nominal_data","OnlyX");}
  }
 }
 std::vector<std::vector<std::string>> groups()const{
  std::vector<std::vector<std::string>> out{{"NOSYS"}};
  for(const auto& [name,cs]:recipes){
   std::vector<std::string> group;
   bool jer=cs.front().resolution;
   for(int data=0;data<=(jer?1:0);++data)for(const auto& sign:{"up","down"})group.push_back("JET_"+name+(data?"_PseudoData":"")+"__1"+sign);
   out.push_back(std::move(group));
  }
  return out;
 }
 std::vector<std::string> names()const{
  std::vector<std::string> out;for(const auto& group:groups())out.insert(out.end(),group.begin(),group.end());return out;
 }
private:
 struct Variation {const std::vector<CompiledComponent>* components;double sign;bool data;};
 Variation resolve(const std::string& sys)const{
  if(sys.rfind("JET_",0)!=0)throw std::invalid_argument("Invalid systematic name");
  auto split=sys.find("__1");if(split==std::string::npos)throw std::invalid_argument("Invalid systematic direction");
  std::string name=sys.substr(4,split-4),direction=sys.substr(split+3);
  if(direction!="up"&&direction!="down")throw std::invalid_argument("Unsupported systematic direction");
  double sign=direction=="up"?1.:-1.;bool data=false;
  const std::string suffix="_PseudoData";
  if(name.size()>suffix.size()&&name.substr(name.size()-suffix.size())==suffix){data=true;name.resize(name.size()-suffix.size());}
  return {&recipes.at(name),sign,data};
 }
 std::array<double,2> amounts(const Variation& variation,const std::vector<double>& r)const{
  if(r.size()!=7)throw std::invalid_argument("Systematics requires seven inputs");
  const auto& cs=*variation.components;
  const double pt=r[0]*double(float(.001)),eta=r[1];
  double unc=0;for(const auto& c:cs)unc+=component(c,pt,eta,r[4],r[5],r[6]!=0);
  double sigma=cs.front().resolution?std::max(jerMC(pt,std::abs(eta)),jerData(pt,std::abs(eta))):0.;
  return {unc,sigma};
 }
 std::vector<double> apply(const Variation& variation,const std::array<double,2>& prepared,const std::vector<double>& r)const{
  double scale=1+variation.sign*prepared[0];
  if(variation.components->front().resolution)scale=smear(r[2],prepared[1],variation.sign*prepared[0],variation.data);
  else if(variation.data)throw std::invalid_argument("PseudoData is only supported for resolution variations");
  return {double(float(r[0]*scale)),r[1],r[2],double(float(r[3]*scale)),0.};
 }
public:
 // Both scalar B evaluation and grouped C evaluation use the same arithmetic.
 std::vector<double> evaluate(const std::string& sys,const std::vector<double>& r)const{
  if(r.size()!=7)throw std::invalid_argument("Systematics requires seven inputs");
  if(sys=="NOSYS")return {r[0],r[1],r[2],r[3],0.};
  const auto variation=resolve(sys);return apply(variation,amounts(variation,r),r);
 }
 std::vector<std::vector<std::vector<double>>> batch_group(const std::vector<std::string>& names,const std::vector<std::vector<double>>& rows)const{
  if(names.empty())throw std::invalid_argument("Empty systematic group");
  if(names.front()=="NOSYS"){
   if(names.size()!=1)throw std::invalid_argument("NOSYS must be evaluated alone");
   return {batch("NOSYS",rows)};
  }
  std::vector<Variation> variations;for(const auto& name:names)variations.push_back(resolve(name));
  for(const auto& variation:variations)if(variation.components!=variations.front().components)throw std::invalid_argument("Group must use one uncertainty recipe");
  std::vector<std::vector<std::vector<double>>> out(names.size());for(auto& state:out)state.reserve(rows.size());
  for(const auto& row:rows){
   const auto prepared=amounts(variations.front(),row);
   for(size_t i=0;i<variations.size();++i)out[i].push_back(apply(variations[i],prepared,row));
  }
  return out;
 }
 std::vector<std::vector<double>> batch(const std::string& sys,const std::vector<std::vector<double>>& rows)const{
  std::vector<std::vector<double>> out;out.reserve(rows.size());for(const auto& r:rows)out.push_back(evaluate(sys,r));return out;
 }
};
