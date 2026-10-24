from pathlib import Path
r=Path('/home/gwatts/atlas/correctionlib_tests')
p=r/'src/systematics.hxx';s=p.read_text()
s=s.replace(' struct CompiledComponent {',''' struct Table {
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
  Table first,second;''')
start=s.index(' double table(');end=s.index(' double component(',start)
s=s[:start]+s[end:]
s=s.replace('  auto h=[&](const std::string& key){return table(key,pt,y,mode);};','')
s=s.replace('h(hist)','c.first(pt,y)')
s=s.replace("auto split=hist.find(',');return dm*h(hist.substr(0,split))+dn*h(hist.substr(split+1));",'return dm*c.first(pt,y)+dn*c.second(pt,y);')
s=s.replace('table("gluonFraction",pt,std::abs(eta),"Full",1)','gluonFraction(pt,std::abs(eta))')
s=s.replace("auto split=hist.find(',');double rg=h(hist.substr(0,split)),rq=h(hist.substr(split+1));",'double rg=c.first(pt,y),rq=c.second(pt,y);')
s=s.replace('table("gluonFractionError",pt,std::abs(eta),"Full",1)','gluonFractionError(pt,std::abs(eta))')
needle='''  }
 }
 std::vector<std::string> names()const{'''
assert needle in s
s=s.replace(needle,'''  }
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
 std::vector<std::string> names()const{''')
s=s.replace('table("JER_Nominal_MC16",pt,std::abs(eta),"OnlyX"),table("JER_Nominal_data",pt,std::abs(eta),"OnlyX")','jerMC(pt,std::abs(eta)),jerData(pt,std::abs(eta))')
assert 'h(hist' not in s and 'table(' not in s
p.write_text(s)
p=r/'src/efficiency.hxx';s=p.read_text()
start=s.index(' double table(');end=s.index(' bool inside(',start)
s=s[:start]+''' correction::Correction::Ref sfValue,sfError,effValue,effError;
 double table(const correction::Correction::Ref& c,double pt,double eta)const{
  return c->evaluate({pt,eta});
 }
'''+s[end:]
needle='  std::ifstream input(config);'
assert needle in s
s=s.replace(needle,'''  sfValue=tables->at("file2__JvtFixedEffPt__None__content");
  sfError=tables->at("file2__JvtFixedEffPt__None__error");
  effValue=tables->at("file2__EffFixedEffPt__None__content");
  effError=tables->at("file2__EffFixedEffPt__None__error");
  std::ifstream input(config);''')
for a,b in [('"JvtFixedEffPt","content"','sfValue'),('"JvtFixedEffPt","error"','sfError'),('"EffFixedEffPt","content"','effValue'),('"EffFixedEffPt","error"','effError')]:s=s.replace('table('+a+',','table('+b+',')
p.write_text(s)
