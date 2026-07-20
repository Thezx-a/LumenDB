cmake_minimum_required(VERSION 3.14)
project(pq_quantization_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(pq_demo
    main.cpp
    pq.cpp
)

target_include_directories(pq_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
