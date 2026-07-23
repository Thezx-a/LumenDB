cmake_minimum_required(VERSION 3.20)
project(TitanKV VERSION 0.1.0 LANGUAGES CXX)

# =========================================================
# TitanKV — Distributed Key-Value Storage Platform
# Top-level CMake: aggregates MiniKV (C++17) + SkyNet (C++20).
# =========================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(ENABLE_TESTS "Enable unit tests (minikv + skynet + course)" ON)
option(ENABLE_BENCHMARKS "Enable benchmarks" OFF)
option(ENABLE_SANITIZERS "Enable Address/Thread sanitizers" OFF)

# Help FetchContent projects (snappy/zstd) on newer CMake.
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# Prefer Ninja when available (faster incremental builds).
# Users can still: cmake -B build -G Ninja ...

# Storage engine (MiniKV LSM-Tree core).
add_subdirectory(minikv)

# Coroutine network library (requires C++20 on the skynet target).
add_subdirectory(skynet)

# Course hand-write unit tests (independent of minikv sources).
if(ENABLE_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/course/CMakeLists.txt")
    enable_testing()
    add_subdirectory(tests/course)
endif()

message(STATUS "TitanKV configured: tests=${ENABLE_TESTS}, type=${CMAKE_BUILD_TYPE}")
