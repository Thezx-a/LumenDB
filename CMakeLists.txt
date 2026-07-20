cmake_minimum_required(VERSION 3.20)
project(DeepVectorMonorepo VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(ENABLE_TESTS "Enable unit tests" OFF)
option(ENABLE_BENCHMARKS "Enable benchmarks" OFF)
option(ENABLE_PYTHON "Enable Python bindings (pybind11)" OFF)

# Integrated build: MiniKV + SkyNet + DeepVector DB
add_subdirectory(deepvector)

# Note: deepvector already pulls in minikv and skynet as dependencies.
# Do not also add_subdirectory(minikv|skynet) here — that would double-define targets.
# Build those standalone from their own directories when needed:
#   cmake -S minikv -B build-minikv ...
#   cmake -S skynet -B build-skynet ...
