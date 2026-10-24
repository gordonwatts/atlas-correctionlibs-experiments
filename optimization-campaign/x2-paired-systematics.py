from pathlib import Path
r=Path('/home/gwatts/atlas/correctionlib_tests')
p=r/'src/systematics.hxx';s=p.read_text()
start=s.index(' std::vector<std::string> names()const{')
end=s.index(' // pt,eta,phi,m,mu,npv,isBjet.',start)
s=s[:start]+''' std::vector<std::vector<std::string>> groups()const{
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
'''+s[end:]
start=s.index(' // pt,eta,phi,m,mu,npv,isBjet.')
end=s.index('  if(sys.rfind("JET_",0)!=0)',start)
s=s[:start]+s[end:]
needle='  const auto& cs=recipes.at(name);\n  const double pt='
assert needle in s
s=s.replace(needle,'''  return {&recipes.at(name),sign,data};
 }
 std::array<double,2> amounts(const Variation& variation,const std::vector<double>& r)const{
  if(r.size()!=7)throw std::invalid_argument("Systematics requires seven inputs");
  const auto& cs=*variation.components;
  const double pt=''')
start=s.index('  double scale=1+sign*unc;');end=s.index('  return {double(float(r[0]*scale))',start)
s=s[:start]+'''  double sigma=cs.front().resolution?std::max(jerMC(pt,std::abs(eta)),jerData(pt,std::abs(eta))):0.;
  return {unc,sigma};
 }
 std::vector<double> apply(const Variation& variation,const std::array<double,2>& prepared,const std::vector<double>& r)const{
  double scale=1+variation.sign*prepared[0];
  if(variation.components->front().resolution)scale=smear(r[2],prepared[1],variation.sign*prepared[0],variation.data);
  else if(variation.data)throw std::invalid_argument("PseudoData is only supported for resolution variations");
'''+s[end:]
needle=' std::vector<std::vector<double>> batch('
pos=s.index(needle)
s=s[:pos]+'''public:
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
'''+s[pos:]
p.write_text(s)
p=r/'src/nominal.cxx';s=p.read_text()
needle='PYBIND11_MODULE(_atlas_corrections,m){'
s=s.replace(needle,'''py::array_t<double> groupArray(const JetSystematics& engine,const std::vector<std::string>& names,Array input){
 if(input.ndim()!=2 || input.shape(1)!=7)throw std::invalid_argument("Expected a two-dimensional seven-column batch");
 const auto n=input.shape(0);const auto states=static_cast<py::ssize_t>(names.size());
 py::array_t<double> out({states,n,py::ssize_t(5)});
 const double* source=input.data();double* target=out.mutable_data();
 {
  py::gil_scoped_release release;
  std::vector<std::vector<double>> rows;rows.reserve(n);
  for(py::ssize_t i=0;i<n;++i)rows.emplace_back(source+i*7,source+(i+1)*7);
  const auto values=engine.batch_group(names,rows);
  for(py::ssize_t k=0;k<states;++k)for(py::ssize_t i=0;i<n;++i)std::copy(values[k][i].begin(),values[k][i].end(),target+(k*n+i)*5);
 }
 return out;
}

'''+needle)
s=s.replace('.def("names",&JetSystematics::names)', '.def("names",&JetSystematics::names).def("groups",&JetSystematics::groups).def("batch_group_array",&groupArray)')
p.write_text(s)
p=r/'python/atlas_correctionlib/parallel.py';s=p.read_text().replace('def batch(evaluate, rows):','def batch(evaluate, rows, axis=0):').replace(')),axis=0)', ')),axis=axis)');p.write_text(s)
p=r/'python/atlas_correctionlib/systematics.py';s=p.read_text()
old='''  for name in self.engine.names():
   values=np.asarray(parallel_batch(lambda chunk:self.engine.batch_array(name,chunk),rows))
   if np.any(values[:,4]!=0):raise RuntimeError("Unsuccessful systematic correction status")
   yield name,values[:,:4]'''
assert old in s
s=s.replace(old,'''  for names in self.engine.groups():
   grouped=np.asarray(parallel_batch(lambda chunk:self.engine.batch_group_array(names,chunk),rows,axis=1))
   if np.any(grouped[:,:,4]!=0):raise RuntimeError("Unsuccessful systematic correction status")
   for name,values in zip(names,grouped):yield name,values[:,:4]''')
p.write_text(s)
