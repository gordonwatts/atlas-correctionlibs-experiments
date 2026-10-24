#pragma once
#include <nlohmann/json.hpp>
class JvtEfficiency {
 std::unique_ptr<correction::CorrectionSet> tables;
 std::map<std::string,std::array<double,4>> bounds;
 correction::Correction::Ref sfValue,sfError,effValue,effError;
 double table(const correction::Correction::Ref& c,double pt,double eta)const{
  return c->evaluate({pt,eta});
 }
 bool inside(const std::string& h,float pt,float eta)const{
  const auto& b=bounds.at(h);return pt>=b[0]&&pt<b[1]&&eta>=b[2]&&eta<b[3];
 }
public:
 JvtEfficiency(const std::string& payload,const std::string& config):tables(correction::CorrectionSet::from_file(payload)){
  sfValue=tables->at("file2__JvtFixedEffPt__None__content");
  sfError=tables->at("file2__JvtFixedEffPt__None__error");
  effValue=tables->at("file2__EffFixedEffPt__None__content");
  effError=tables->at("file2__EffFixedEffPt__None__error");
  std::ifstream input(config);nlohmann::json j;input>>j;
  for(const auto& h:{"JvtFixedEffPt","EffFixedEffPt"}){
   bool found=false;
   for(auto it=j.at("inventory").begin();it!=j.at("inventory").end();++it){
    std::string key=it.key(),suffix=std::string("::")+h;
    if(key.size()>=suffix.size()&&key.substr(key.size()-suffix.size())==suffix){
     bounds[h]=it.value().at("bounds").get<std::array<double,4>>();found=true;
    }
   }
   if(!found)throw std::runtime_error("Missing JVT histogram bounds");
  }
 }
 // Returns weight, status (0=Ok, 1=OutOfValidityRange).
 std::vector<double> evaluate(float pt,float eta,bool pass,int sigma)const{
  if(!inside("JvtFixedEffPt",pt,eta)||(!pass&&!inside("EffFixedEffPt",pt,eta)))return {-1.,1.};
  float base=table(sfValue,pt,eta),error=table(sfError,pt,eta);
  base+=error*sigma;
  if(pass)return {double(base),0.};
  float eff=table(effValue,pt,eta),errorEff=table(effError,pt,eta);
  eff+=errorEff*sigma;
  float sf=(1-base*eff)/(1-eff);
  return {double(sf),0.};
 }
 std::vector<double> evaluate_row(const std::vector<double>& r,int sigma)const{
  if(r.size()!=4)throw std::invalid_argument("JVT efficiency expects pt, eta, pass, truthHS");
  if(r[0]<20000||r[0]>60000||std::abs(float(r[1]))>2.5)return {-1.,1.};
  if(r[3]==0)return {1.,0.};
  return evaluate(float(r[0]),float(r[1]),r[2]!=0,sigma);
 }
 std::vector<std::vector<double>> batch(const std::vector<std::vector<double>>& rows,int sigma)const{
  std::vector<std::vector<double>> out;out.reserve(rows.size());
  for(const auto& r:rows)out.push_back(evaluate_row(r,sigma));
  return out;
 }
};
