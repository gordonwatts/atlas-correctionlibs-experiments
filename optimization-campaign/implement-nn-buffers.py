from pathlib import Path
import shutil
r=Path('/home/gwatts/atlas/correctionlib_tests');w=Path('/mnt/c/Users/gwatts/OneDrive/Documents/ATLAS/Computing/AMG/CorrectionLib')
shutil.copy2(w/'nn_buffered.hxx',r/'src/nn_buffered.hxx')
p=r/'src/nn_graph.hxx';s=p.read_text().replace('#include <lwtnn/Stack.hh>','#include <lwtnn/Stack.hh>\n#include "nn_buffered.hxx"').replace(' bool supported;',' BufferedNN buffered;\n bool supported;').replace(':graph(cfg,order),',':graph(cfg,order),buffered(cfg,order),')
s=s.replace(' double score4(', ' double uncached_score4(const std::array<double,4>& input)const{\n  return buffered.supported()?buffered.evaluate(input,id):reference_score4(input);\n }\n double score4(')
s=s.replace('if(!supported||!cacheEnabled)return reference_score4(input);','if(!supported)return reference_score4(input);\n  if(!cacheEnabled)return uncached_score4(input);')
s=s.replace('const double value=reference_score4(input);','const double value=uncached_score4(input);')
p.write_text(s)
