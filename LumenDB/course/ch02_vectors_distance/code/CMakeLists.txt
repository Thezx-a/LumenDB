cmake_minimum_required(VERSION 3.16)
project(Chapter02_Distance LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(distance_test
    main.cpp
    distance.cpp
)
target_include_directories(distance_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(distance_test PRIVATE -O3 -mavx2 -mfma)
endif()
