#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "graph.hxx"
namespace py=pybind11;
PYBIND11_MODULE(_declarative,m){
 m.attr("evaluator_version")="correctionlib-2.7.0+graph-1";
 py::class_<correction::GraphSet>(m,"GraphSet").def(py::init<const std::string&>()).def("names",&correction::GraphSet::names).def("evaluate",&correction::GraphSet::evaluate,py::call_guard<py::gil_scoped_release>()).def("batch",&correction::GraphSet::batch,py::call_guard<py::gil_scoped_release>());
}
