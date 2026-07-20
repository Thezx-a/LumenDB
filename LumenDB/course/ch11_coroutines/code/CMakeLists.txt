cmake_minimum_required(VERSION 3.16)
project(lumendb_coroutines VERSION 0.1.0 LANGUAGES CXX)

add_executable(coroutine_demo main.cpp)

target_compile_features(coroutine_demo PRIVATE cxx_std_20)
