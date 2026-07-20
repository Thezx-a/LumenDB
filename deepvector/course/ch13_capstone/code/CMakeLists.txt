cmake_minimum_required(VERSION 3.16)
project(mini_vdb VERSION 0.1.0 LANGUAGES CXX)

add_executable(mini_vdb
    main.cpp
    vector_db.cpp
)

target_compile_features(mini_vdb PRIVATE cxx_std_17)
target_compile_options(mini_vdb PRIVATE -O3 -march=native)
