cmake_minimum_required(VERSION 3.16)
project(lumendb_benchmark VERSION 0.1.0 LANGUAGES CXX)

add_executable(benchmark benchmark.cpp)

target_compile_features(benchmark PRIVATE cxx_std_17)
target_compile_options(benchmark PRIVATE -O3 -march=native)
