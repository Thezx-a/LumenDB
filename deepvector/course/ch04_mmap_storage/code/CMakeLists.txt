cmake_minimum_required(VERSION 3.16)
project(Chapter04_Mmap LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(mmap_test
    main.cpp
    mmap_array.cpp
)
target_include_directories(mmap_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mmap_test PRIVATE -O3)
endif()
