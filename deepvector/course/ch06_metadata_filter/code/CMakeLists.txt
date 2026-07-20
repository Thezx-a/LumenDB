cmake_minimum_required(VERSION 3.14)
project(metadata_filter_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(filter_demo
    main.cpp
    filter.cpp
)

target_include_directories(filter_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
