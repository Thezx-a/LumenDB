cmake_minimum_required(VERSION 3.16)
project(deepvector_http VERSION 0.1.0 LANGUAGES CXX)

add_executable(deepvector_http
    main.cpp
    server.cpp
)

target_compile_features(deepvector_http PRIVATE cxx_std_17)
target_link_libraries(deepvector_http PRIVATE pthread)
