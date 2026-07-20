cmake_minimum_required(VERSION 3.16)
project(Chapter03_HNSW LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(hnsw_test
    main.cpp
    hnsw.cpp
)
target_include_directories(hnsw_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(hnsw_test PRIVATE -O3)
endif()
