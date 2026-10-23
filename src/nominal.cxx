#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include "backend.hxx"
#include "nnjvt.hxx"
namespace py=pybind11;

// Copy dense input/output in native code; retain the shared scalar evaluator.
template<class F>
py::array_t<double> arrayBatch(py::array_t<double, py::array::c_style | py::array::forcecast> input,
                               py::ssize_t columns, py::ssize_t outputs, F evaluate) {
 if(input.ndim()!=2 || input.shape(1)!=columns)
  throw std::invalid_argument("Expected a two-dimensional batch with the declared input width");
 const auto n=input.shape(0);
 py::array_t<double> output({n,outputs});
 const double* source=input.data();double* target=output.mutable_data();
 {
  py::gil_scoped_release release;
  std::vector<std::vector<double>> rows;rows.reserve(n);
  for(py::ssize_t i=0;i<n;++i)rows.emplace_back(source+i*columns,source+(i+1)*columns);
  auto values=evaluate(rows);
  if(values.size()!=static_cast<size_t>(n))throw std::runtime_error("Unexpected batch output length");
  for(py::ssize_t i=0;i<n;++i){
   if(values[i].size()!=static_cast<size_t>(outputs))throw std::runtime_error("Unexpected batch output width");
   std::copy(values[i].begin(),values[i].end(),target+i*outputs);
  }
 }
 return output;
}
using Array=py::array_t<double, py::array::c_style | py::array::forcecast>;

PYBIND11_MODULE(_atlas_corrections,m){
 py::class_<JvtEfficiency>(m,"JvtEfficiency").def(py::init<const std::string&,const std::string&>()).def("evaluate",&JvtEfficiency::evaluate).def("batch_array",[](const JvtEfficiency& self,Array a,int sigma){return arrayBatch(a,4,2,[&](const auto& rows){return self.batch(rows,sigma);});}).def("batch",&JvtEfficiency::batch,py::call_guard<py::gil_scoped_release>());
 py::class_<JetSystematics>(m,"JetSystematics").def(py::init<const std::string&,const std::string&>()).def("names",&JetSystematics::names).def("evaluate",&JetSystematics::evaluate,py::call_guard<py::gil_scoped_release>()).def("batch_array",[](const JetSystematics& self,const std::string& sys,Array a){return arrayBatch(a,7,5,[&](const auto& rows){return self.batch(sys,rows);});}).def("batch",&JetSystematics::batch,py::call_guard<py::gil_scoped_release>());
 py::class_<NNJvt>(m,"NNJvt").def(py::init<const std::string&,std::vector<float>,std::vector<float>,std::vector<std::vector<float>>>()).def("batch_array",[](const NNJvt& self,Array a){return arrayBatch(a,6,2,[&](const auto& rows){return self.batch(rows);});}).def("batch",&NNJvt::batch,py::call_guard<py::gil_scoped_release>());
 m.def("smear_batch",&smearBatch,py::call_guard<py::gil_scoped_release>());

 py::class_<Nominal>(m,"Nominal").def(py::init<const std::string&,double,double,double>())
 .def("evaluate",&Nominal::evaluate,py::call_guard<py::gil_scoped_release>())
 .def("batch_array",[](const Nominal& self,Array a){return arrayBatch(a,14,12,[&](const auto& rows){return self.batch(rows);});}).def("batch",&Nominal::batch,py::call_guard<py::gil_scoped_release>());
}
