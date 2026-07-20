#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "VectorStorage.h"

namespace py = pybind11;

float l2_distance(py::array_t<float> a, py::array_t<float> b) {
    py::buffer_info a_info = a.request();
    py::buffer_info b_info = b.request();

    if (a_info.ndim != 1 || b_info.ndim != 1)
        throw std::runtime_error("Expected 1D arrays");
    if (a_info.shape[0] != b_info.shape[0])
        throw std::runtime_error("Dimension mismatch");

    float* a_ptr = static_cast<float*>(a_info.ptr);
    float* b_ptr = static_cast<float*>(b_info.ptr);
    ssize_t dim = a_info.shape[0];

    float sum = 0.0f;
    for (ssize_t i = 0; i < dim; i++) {
        float diff = a_ptr[i] - b_ptr[i];
        sum += diff * diff;
    }
    return std::sqrt(sum);
}

std::vector<float> scale_vector(const std::vector<float>& vec, float factor) {
    std::vector<float> result;
    result.reserve(vec.size());
    for (float v : vec) result.push_back(v * factor);
    return result;
}

PYBIND11_MODULE(lumen_core, m) {
    m.doc() = "DeepVector Python bindings";

    m.def("l2_distance", &l2_distance, "Compute L2 distance between two numpy arrays",
          py::arg("a"), py::arg("b"));

    m.def("scale_vector", &scale_vector, "Scale a vector by a factor",
          py::arg("vec"), py::arg("factor"));

    py::class_<VectorStorage>(m, "VectorStorage")
        .def(py::init<size_t, size_t>(),
             py::arg("dimension"), py::arg("capacity") = 10000)
        .def("append", &VectorStorage::append, "Append a vector, returns its id",
             py::arg("vector"))
        .def("get_vector", &VectorStorage::get_vector, "Get vector by id",
             py::arg("id"))
        .def("count", &VectorStorage::count, "Number of stored vectors")
        .def("dimension", &VectorStorage::dimension, "Vector dimension")
        .def("save", &VectorStorage::save, "Persist to disk",
             py::arg("path"))
        .def_static("load", &VectorStorage::load, "Load from disk",
             py::arg("path"));
}
