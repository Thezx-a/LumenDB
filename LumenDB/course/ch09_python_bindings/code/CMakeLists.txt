cmake_minimum_required(VERSION 3.16)
project(lumen_core VERSION 0.1.0 LANGUAGES CXX)

find_package(pybind11 REQUIRED)

pybind11_add_module(lumen_core
    bindings.cpp
    VectorStorage.cpp
)

target_compile_features(lumen_core PRIVATE cxx_std_17)
