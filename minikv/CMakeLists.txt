cmake_minimum_required(VERSION 3.16)
project(MiniKV VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(ENABLE_TESTS "Enable unit tests" OFF)
option(ENABLE_BENCHMARKS "Enable benchmarks" OFF)
option(ENABLE_SANITIZERS "Enable AddressSanitizer and ThreadSanitizer" OFF)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerWarnings.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Sanitizers.cmake)

add_library(minikv STATIC
    src/core/wal.cpp
    src/core/memtable.cpp
    src/core/sstable_builder.cpp
    src/core/sstable_reader.cpp
    src/core/block.cpp
    src/core/compaction.cpp
    src/core/version.cpp
    src/core/db_impl.cpp
    src/core/status.cpp
    src/network/event_loop.cpp
    src/network/server.cpp
    src/network/connection.cpp
    src/utils/crc32.cpp
    src/utils/env_posix.cpp
)

target_include_directories(minikv PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(minikv PUBLIC cxx_std_17)
apply_compiler_warnings(minikv)
apply_sanitizers(minikv)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    add_executable(minikv_server src/main.cpp)
    target_link_libraries(minikv_server PRIVATE minikv)
endif()

# Tests and benchmarks only when MiniKV is the top-level project
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    if(ENABLE_TESTS)
        include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchGoogleTest.cmake)
        enable_testing()
        add_subdirectory(tests)
    endif()

    if(ENABLE_BENCHMARKS)
        add_subdirectory(benches)
    endif()
endif()