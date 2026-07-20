cmake_minimum_required(VERSION 3.16)
project(DeepVector VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(ENABLE_TESTS "Enable unit tests" OFF)
option(ENABLE_BENCHMARKS "Enable benchmarks" OFF)
option(ENABLE_PYTHON "Enable Python bindings (pybind11)" OFF)
option(USE_AVX2 "Enable AVX2 SIMD acceleration" ON)

include(CheckCXXCompilerFlag)
if(USE_AVX2)
    check_cxx_compiler_flag(-mavx2 COMPILER_SUPPORTS_AVX2)
    check_cxx_compiler_flag(-mfma COMPILER_SUPPORTS_FMA)
    if(COMPILER_SUPPORTS_AVX2 AND COMPILER_SUPPORTS_FMA)
        add_compile_options(-mavx2 -mfma)
    endif()
endif()

add_subdirectory(../minikv minikv)
add_subdirectory(../skynet skynet)

include(FetchContent)
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

add_library(deepvector STATIC
    src/collection.cpp
    src/filter.cpp
    src/index/hnsw.cpp
    src/quantize/pq.cpp
    src/quantize/scalar.cpp
    src/storage/vector_store.cpp
    src/storage/document_store.cpp
)

target_include_directories(deepvector PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_options(deepvector PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wno-unused-parameter>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wno-unused-parameter>
)

find_package(Threads REQUIRED)
target_link_libraries(deepvector PUBLIC Threads::Threads minikv)

if(ENABLE_TESTS)
    include(FetchContent)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
    enable_testing()
    add_subdirectory(tests)
endif()

if(ENABLE_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(ENABLE_PYTHON)
    add_subdirectory(python)
endif()

add_executable(deepvector_server
    src/server/main.cpp
    src/server/server.cpp
)
target_compile_features(deepvector_server PRIVATE cxx_std_20)
target_link_libraries(deepvector_server PRIVATE deepvector skynet nlohmann_json::nlohmann_json)
target_include_directories(deepvector_server PRIVATE ${CMAKE_SOURCE_DIR}/include)
