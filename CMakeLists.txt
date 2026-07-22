cmake_minimum_required(VERSION 3.20)
project(TitanKV VERSION 0.1.0 LANGUAGES CXX)

# =========================================================
# TitanKV — Distributed Key-Value Storage Platform
# Top-level CMake entry.
#
# Components:
#   - minikv   : C++17 LSM-Tree KV engine (core storage engine)
#   - skynet   : C++20 coroutine network library (standalone)
# =========================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(ENABLE_TESTS "Enable unit tests" OFF)
option(ENABLE_BENCHMARKS "Enable benchmarks" OFF)
option(ENABLE_SANITIZERS "Enable Address/Thread sanitizers" OFF)

# Storage engine (MiniKV LSM-Tree core).
add_subdirectory(minikv)

# SkyNet: C++20 coroutine network library (kept as standalone, linked by gRPC server later)
if(TARGET skynet)
    # nothing — skynet builds its own static lib when built standalone
endif()

# =========================================================
# Course module hands-on unit tests (TitanKV curriculum).
# Independent of minikv — fetches GoogleTest itself.
# =========================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/course/CMakeLists.txt")
    enable_testing()
    add_subdirectory(tests/course)
endif()