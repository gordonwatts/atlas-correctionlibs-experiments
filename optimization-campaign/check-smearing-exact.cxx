#include "backend.hxx"
#include <iostream>
double reference(double phi,double nominal,double variation,bool data){
 if(variation==0 || (data && variation>0) || (!data && variation<0))return 1.;
 const double width=std::sqrt(std::pow(nominal+std::abs(variation),2)-std::pow(nominal,2));
 long long seed=(data?1.23e5:2.00e5)*std::abs(phi);
 if(!seed)seed=data?34545654:45583453;
 TRandom3 random;random.SetSeed(seed);
 double value=-1;while(value<0)value=random.Gaus(1.,width);return value;
}
int main(){
 size_t checked=0;
 // Sweep the physical phi range, zero/fallback seeds, and deliberately
 // out-of-range phi values to exercise cache collisions. Large widths force
 // rejection, and signs include both inactive and active smearing states.
 for(int pass=0;pass<2;++pass)for(int i=0;i<24000;++i){
  int j=pass?23999-i:i;
  double phi=(j%17==0)?0.:(j%19==0?1e-9:(j%23==0?8.4:-3.14159+6.28318*j/24000.));
  double nominal=(j%7)*.2;
  double variation=(j%11==0?10.:(j%13==0?0.:.01*(1+j%80)))*(j%2?1.:-1.);
  for(bool data:{false,true}){
   const auto expected=reference(phi,nominal,variation,data);
   const auto actual=smear(phi,nominal,variation,data);
   if(expected!=actual){std::cerr<<"Mismatch "<<j<<" "<<data<<"\n";return 1;}
   ++checked;
  }
 }
 for(bool data:{false,true})for(int repeat=0;repeat<10;++repeat){
  const double factor=data?123000.:200000.;
  for(double seed:{100000.25,1148576.25,100000.25}){
   double phi=seed/factor,variation=data?-20.:20.;
   if(reference(phi,.1,variation,data)!=smear(phi,.1,variation,data))return 2;
   ++checked;
  }
 }
 std::cout<<"PASS: "<<checked<<" bitwise-equal ROOT reference draws, including rejection and changed traversal order\n";
}
