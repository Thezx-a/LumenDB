cmake_minimum_required(VERSION 3.16)
project(lumendb_http VERSION 0.1.0 LANGUAGES CXX)

add_executable(lumendb_http
    main.cpp
    server.cpp
)

target_compile_features(lumendb_http PRIVATE cxx_std_17)
target_link_libraries(lumendb_http PRIVATE pthread)
