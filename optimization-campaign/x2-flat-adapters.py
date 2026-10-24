from pathlib import Path
r=Path('/home/gwatts/atlas/correctionlib_tests')
p=r/'src/nnjvt.hxx';s=p.read_text()
a=s.index(' std::vector<std::vector<double>> batch(');body=s.index('   if(r.size()!=6)',a);b=s.index('   out.push_back(',body)
logic=s[body:b]
s=s[:a]+''' std::array<double,2> evaluate_row(const std::vector<double>& r) const {
'''+logic+'''   return {score,(!inRange||score>cut)?1.:0.};
 }
 std::vector<std::vector<double>> batch(const std::vector<std::vector<double>>& rows) const {
  std::vector<std::vector<double>> out;out.reserve(rows.size());
  for(const auto& r:rows){const auto result=evaluate_row(r);out.emplace_back(result.begin(),result.end());}
  return out;
 }
};
'''
s='#include <array>\n'+s;p.write_text(s)
p=r/'src/efficiency.hxx';s=p.read_text();a=s.index(' std::vector<std::vector<double>> batch(')
s=s[:a]+''' std::vector<double> evaluate_row(const std::vector<double>& r,int sigma)const{
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
''';p.write_text(s)
p=r/'src/nominal.cxx';s=p.read_text()
a=s.index('  std::vector<std::vector<double>> rows;rows.reserve(n);');b=s.index('\n }\n return output;',a)
s=s[:a]+'''  std::vector<double> row(columns);
  for(py::ssize_t i=0;i<n;++i){
   std::copy_n(source+i*columns,columns,row.begin());
   const auto values=evaluate(row);
   if(values.size()!=static_cast<size_t>(outputs))throw std::runtime_error("Unexpected batch output width");
   std::copy(values.begin(),values.end(),target+i*outputs);
  }'''+s[b:]
s=s.replace('self.batch(rows,sigma)','self.evaluate_row(rows,sigma)').replace('self.batch(sys,rows)','self.evaluate(sys,rows)')
s=s.replace('self.batch(rows);','self.evaluate_row(rows);',1)
s=s.replace('self.batch(rows);','self.evaluate(rows);',1)
p.write_text(s)
