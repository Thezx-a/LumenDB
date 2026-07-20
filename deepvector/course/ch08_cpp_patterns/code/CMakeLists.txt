cmake_minimum_required(VERSION 3.14)
project(cpp_patterns_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(patterns_demo
    main.cpp
    pimpl.cpp
    thread_pool.cpp
)

target_include_directories(patterns_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(patterns_demo PRIVATE Threads::Threads)
