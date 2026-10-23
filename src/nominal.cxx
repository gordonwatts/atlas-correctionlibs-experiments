#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "backend.hxx"
#include "nnjvt.hxx"
namespace py=pybind11;
PYBIND11_MODULE(_atlas_corrections,m){
 py::class_<JvtEfficiency>(m,"JvtEfficiency").def(py::init<const std::string&,const std::string&>()).def("evaluate",&JvtEfficiency::evaluate).def("batch",&JvtEfficiency::batch,py::call_guard<py::gil_scoped_release>());
 py::class_<JetSystematics>(m,"JetSystematics").def(py::init<const std::string&,const std::string&>()).def("names",&JetSystematics::names).def("evaluate",&JetSystematics::evaluate,py::call_guard<py::gil_scoped_release>()).def("batch",&JetSystematics::batch,py::call_guard<py::gil_scoped_release>());
 py::class_<NNJvt>(m,"NNJvt").def(py::init<const std::string&,std::vector<float>,std::vector<float>,std::vector<std::vector<float>>>()).def("batch",&NNJvt::batch,py::call_guard<py::gil_scoped_release>());
 m.def("smear_batch",&smearBatch,py::call_guard<py::gil_scoped_release>());

 py::class_<Nominal>(m,"Nominal").def(py::init<const std::string&,double,double,double>())
 .def("evaluate",&Nominal::evaluate,py::call_guard<py::gil_scoped_release>())
 .def("batch",&Nominal::batch,py::call_guard<py::gil_scoped_release>());
}
