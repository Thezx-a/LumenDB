#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include "dv/collection.h"
#include "dv/filter.h"

namespace py = pybind11;
using namespace lumendb;

PYBIND11_MODULE(_deepvector, m) {
    m.doc() = "DeepVector - C++ Zero-Copy Vector Database for RAG";

    py::enum_<DistanceMetric>(m, "DistanceMetric")
        .value("L2", DistanceMetric::L2)
        .value("InnerProduct", DistanceMetric::InnerProduct)
        .value("Cosine", DistanceMetric::Cosine)
        .export_values();

    py::class_<SearchResult>(m, "SearchResult")
        .def(py::init<>())
        .def_readwrite("id", &SearchResult::id)
        .def_readwrite("distance", &SearchResult::distance)
        .def("__repr__", [](const SearchResult& r) {
            return "<SearchResult id=" + std::to_string(r.id) +
                   " dist=" + std::to_string(r.distance) + ">";
        });

    py::class_<CollectionConfig>(m, "CollectionConfig")
        .def(py::init<>())
        .def_readwrite("dim", &CollectionConfig::dim)
        .def_readwrite("metric", &CollectionConfig::metric)
        .def_readwrite("hnsw_m", &CollectionConfig::hnsw_m)
        .def_readwrite("hnsw_ef_construction", &CollectionConfig::hnsw_ef_construction)
        .def_readwrite("hnsw_ef_search", &CollectionConfig::hnsw_ef_search)
        .def_readwrite("max_elements", &CollectionConfig::max_elements)
        .def_readwrite("use_pq", &CollectionConfig::use_pq)
        .def_readwrite("pq_M", &CollectionConfig::pq_M)
        .def_readwrite("pq_K", &CollectionConfig::pq_K)
        .def_readwrite("use_sq", &CollectionConfig::use_sq);

    py::class_<storage::DocumentMeta>(m, "DocumentMeta")
        .def(py::init<>())
        .def_readwrite("text", &storage::DocumentMeta::text)
        .def_readwrite("tags", &storage::DocumentMeta::tags)
        .def_readwrite("timestamp", &storage::DocumentMeta::timestamp)
        .def("__repr__", [](const storage::DocumentMeta& meta) {
            return "<DocumentMeta text='" + meta.text + "' tags='" + meta.tags + "'>";
        });

    py::class_<Collection>(m, "Collection")
        .def(py::init<const CollectionConfig&, const std::string&>(),
             py::arg("config"), py::arg("data_dir") = ".")
        .def("add",
             [](Collection& self,
                py::array_t<float, py::array::c_style | py::array::forcecast> vec) {
                 py::buffer_info buf = vec.request();
                 if (buf.ndim != 1) throw std::runtime_error("Vector must be 1D");
                 auto* data = static_cast<const float*>(buf.ptr);
                 py::gil_scoped_release release;
                 return self.add(data);
             },
             py::arg("vector"))
        .def("add_with_meta",
             [](Collection& self,
                py::array_t<float, py::array::c_style | py::array::forcecast> vec,
                const storage::DocumentMeta& meta) {
                 py::buffer_info buf = vec.request();
                 if (buf.ndim != 1) throw std::runtime_error("Vector must be 1D");
                 auto* data = static_cast<const float*>(buf.ptr);
                 py::gil_scoped_release release;
                 return self.add(data, meta);
             },
             py::arg("vector"), py::arg("meta"))
        .def("search",
             [](Collection& self,
                py::array_t<float, py::array::c_style | py::array::forcecast> query,
                size_t k) {
                 py::buffer_info buf = query.request();
                 if (buf.ndim != 1) throw std::runtime_error("Query must be 1D");
                 auto* data = static_cast<const float*>(buf.ptr);
                 py::gil_scoped_release release;
                 return self.search(data, k);
             },
             py::arg("query"), py::arg("k") = 10)
        .def("search_with_filter",
             [](Collection& self,
                py::array_t<float, py::array::c_style | py::array::forcecast> query,
                size_t k, const FilterNode& filter) {
                 py::buffer_info buf = query.request();
                 if (buf.ndim != 1) throw std::runtime_error("Query must be 1D");
                 auto* data = static_cast<const float*>(buf.ptr);
                 py::gil_scoped_release release;
                 return self.searchWithFilter(data, k, filter);
             },
             py::arg("query"), py::arg("k"), py::arg("filter"))
        .def("get_vector",
             [](Collection& self, uint64_t id) -> py::object {
                 const float* v = self.getVector(id);
                 if (!v) return py::none();
                 Dimension d = self.dim();
                 return py::array_t<float>(d, v);
             })
        .def("get_meta",
             [](Collection& self, uint64_t id) -> py::object {
                 auto meta = self.getMeta(id);
                 if (!meta) return py::none();
                 return py::cast(*meta);
             })
        .def("remove", &Collection::remove)
        .def("save", &Collection::save, py::arg("name"))
        .def_static("load", &Collection::load,
                    py::arg("name"), py::arg("data_dir") = ".")
        .def("__len__", &Collection::size)
        .def_property_readonly("dim", &Collection::dim);

    py::class_<FilterNode>(m, "FilterNode")
        .def(py::init<>())
        .def_readwrite("op", &FilterNode::op)
        .def_readwrite("field", &FilterNode::field)
        .def_readwrite("value", &FilterNode::value)
        .def_readwrite("children", &FilterNode::children)
        .def("is_leaf", &FilterNode::isLeaf)
        .def_static("eq", &FilterNode::eq,
                    py::arg("field"), py::arg("value"))
        .def_static("contains", &FilterNode::contains,
                    py::arg("field"), py::arg("value"))
        .def_static("gt", &FilterNode::gt,
                    py::arg("field"), py::arg("value"))
        .def_static("lt", &FilterNode::lt,
                    py::arg("field"), py::arg("value"))
        .def_static("and_", &FilterNode::andAlso,
                    py::arg("a"), py::arg("b"))
        .def_static("or_", &FilterNode::orElse,
                    py::arg("a"), py::arg("b"))
        .def("__repr__", [](const FilterNode& f) {
            return "<FilterNode op=" +
                   std::to_string(static_cast<int>(f.op)) + ">";
        });
}
