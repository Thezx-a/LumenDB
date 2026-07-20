cmake_minimum_required(VERSION 3.14)
project(lsm_tree_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lsm_demo
    main.cpp
    skiplist.cpp
    bloom.cpp
)

target_include_directories(lsm_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
