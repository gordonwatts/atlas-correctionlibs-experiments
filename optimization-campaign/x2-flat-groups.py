from pathlib import Path
r=Path('/home/gwatts/atlas/correctionlib_tests')
p=r/'src/systematics.hxx';s=p.read_text()
s=s.replace(' std::vector<double> apply(', ' std::array<double,5> apply(')
s=s.replace('const auto variation=resolve(sys);return apply(variation,amounts(variation,r),r);','const auto variation=resolve(sys);const auto result=apply(variation,amounts(variation,r),r);return {result.begin(),result.end()};')
s=s.replace('for(size_t i=0;i<variations.size();++i)out[i].push_back(apply(variations[i],prepared,row));','for(size_t i=0;i<variations.size();++i){const auto result=apply(variations[i],prepared,row);out[i].emplace_back(result.begin(),result.end());}')
needle=' std::vector<std::vector<std::vector<double>>> batch_group('
pos=s.index(needle)
s=s[:pos]+''' // Contiguous NumPy storage: no per-jet input or output heap allocations.
 void batch_group_into(const std::vector<std::string>& names,const double* input,size_t n,double* output)const{
  if(names.empty())throw std::invalid_argument("Empty systematic group");
  std::vector<double> row(7);
  if(names.front()=="NOSYS"){
   if(names.size()!=1)throw std::invalid_argument("NOSYS must be evaluated alone");
   for(size_t i=0;i<n;++i){std::copy_n(input+7*i,4,output+5*i);output[5*i+4]=0.;}
   return;
  }
  std::vector<Variation> variations;for(const auto& name:names)variations.push_back(resolve(name));
  for(const auto& variation:variations)if(variation.components!=variations.front().components)throw std::invalid_argument("Group must use one uncertainty recipe");
  for(size_t i=0;i<n;++i){
   std::copy_n(input+7*i,7,row.begin());
   const auto prepared=amounts(variations.front(),row);
   for(size_t k=0;k<variations.size();++k){
    const auto result=apply(variations[k],prepared,row);
    std::copy(result.begin(),result.end(),output+(k*n+i)*5);
   }
  }
 }
'''+s[pos:];p.write_text(s)
p=r/'src/nominal.cxx';s=p.read_text()
a=s.index('  std::vector<std::vector<double>> rows;rows.reserve(n);',s.index('py::array_t<double> groupArray'))
b=s.index('\n }\n return out;',a)
s=s[:a]+'  engine.batch_group_into(names,source,static_cast<size_t>(n),target);'+s[b:]
p.write_text(s)
