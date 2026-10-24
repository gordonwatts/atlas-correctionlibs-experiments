#pragma once
#include <correction.h>
#include <Math/Vector4D.h>
#include <TLorentzVector.h>
#include <TRandom3.h>
#include <array>
#include <cmath>
#include <stdexcept>
#include <fstream>
#include <memory>
using P4=ROOT::Math::PtEtaPhiMVector;
class Nominal {
 std::unique_ptr<correction::CorrectionSet> c;
 double muRef,npvRef,muScale;
 static P4 stored(const P4& p) { return P4(float(p.Pt()),float(p.Eta()),float(p.Phi()),float(p.M())); }
 static void append(std::vector<double>& out,const P4& p) {
  out.insert(out.end(),{p.Pt(),p.Eta(),p.Phi(),p.M()});
 }
public:
 Nominal(const std::string& path,double mr,double nr,double ms):c(correction::CorrectionSet::from_file(path)),muRef(mr),npvRef(nr),muScale(ms){}
 // pt,eta,phi,m,DetectorEta,areaPt,rho,mu,npv,chargedFraction,Tile0,EM3,nTrk,trackWidth
 std::vector<double> evaluate(const std::vector<double>& v) const {
  if(v.size()!=14) throw std::invalid_argument("Expected 14 explicit jet/event inputs");
  if(v[0]<=0 || !std::isfinite(v[0])) throw std::invalid_argument("nonpositive or nonfinite input pt");
  P4 p(v[0],v[1],v[2],v[3]);std::vector<double> out;
  const double a=c->at("residual_mu")->evaluate({std::abs(v[1])});
  const double b=c->at("residual_npv")->evaluate({std::abs(v[1])});
  double offset=1000*(a*(v[7]*muScale-muRef)+b*(v[8]-npvRef));
  double sf=c->at("pileup_scale")->evaluate({p.Pt(),v[5],v[6],offset});
  p=stored(p*sf);append(out,p);
  sf=c->at("jes_scale")->evaluate({p.E()/1000.,v[4],0.});
  P4 q=p*sf;
  double eta=q.Eta()+c->at("eta_shift")->evaluate({q.E()/1000.,v[4]});
  TLorentzVector t;t.SetPtEtaPhiM(q.P()/std::cosh(eta),eta,q.Phi(),q.M());
  P4 shifted;shifted.SetPxPyPzE(t.Px(),t.Py(),t.Pz(),t.E());
  p=stored(shifted);append(out,p);
  sf=c->compound().at("gsc")->evaluate({p.Pt()/1000.,std::abs(v[4]),0.,v[9],v[10],v[11],v[12],v[13]});
  p=stored(p*sf);append(out,p);return out;
 }
 std::vector<std::vector<double>> batch(const std::vector<std::vector<double>>& rows) const {
  std::vector<std::vector<double>> out;out.reserve(rows.size());
  for(const auto& r:rows)out.push_back(evaluate(r));return out;
 }
};
inline double smear(double phi,double sigmaNom,double variation,bool data) {
 if(variation==0 || (data && variation>0) || (!data && variation<0))return 1.;
 const double width=std::sqrt(std::pow(sigmaNom+std::abs(variation),2)-std::pow(sigmaNom,2));
 long long seed=(data?1.23e5:2.00e5)*std::abs(phi);
 if(!seed)seed=data?34545654:45583453;
 TRandom3 random(seed);
 double value=-1;while(value<0)value=random.Gaus(1.,width);return value;
}
inline std::vector<double> smearBatch(const std::vector<double>& phi,const std::vector<double>& sigma,const std::vector<double>& variation,bool data){
 if(phi.size()!=sigma.size()||phi.size()!=variation.size())throw std::invalid_argument("smear array lengths differ");
 std::vector<double> out;out.reserve(phi.size());
 for(size_t i=0;i<phi.size();++i)out.push_back(smear(phi[i],sigma[i],variation[i],data));
 return out;
}
#include "systematics.hxx"

#include "efficiency.hxx"
