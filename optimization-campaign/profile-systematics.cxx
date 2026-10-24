#include "backend.hxx"
#include <valgrind/callgrind.h>
#include <iostream>
int main(int argc,char** argv){
 if(argc!=2)return 2;
 JetSystematics engine("payloads/uncertainty-tables.json.gz","payloads/uncertainty-manifest.json");
 std::ifstream input(argv[1],std::ios::binary);
 std::vector<std::vector<double>> rows;
 for(std::vector<double> row(7);input.read(reinterpret_cast<char*>(row.data()),7*sizeof(double));)rows.push_back(row);
 if(rows.empty())return 3;
 auto names=engine.names();double checksum=0;
 CALLGRIND_START_INSTRUMENTATION;
 for(const auto& name:names){auto values=engine.batch(name,rows);for(const auto& row:values)checksum+=row[0];}
 CALLGRIND_STOP_INSTRUMENTATION;
 CALLGRIND_DUMP_STATS;
 std::cout<<rows.size()<<" jets, "<<names.size()<<" states, checksum "<<checksum<<"\n";
}
