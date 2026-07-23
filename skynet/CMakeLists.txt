cmake_minimum_required(VERSION 3.20)
project(SkyNet VERSION 0.1.0 LANGUAGES CXX)

option(ENABLE_TESTS "Enable unit tests" OFF)
option(ENABLE_BENCHMARKS "Enable benchmarks" OFF)
option(ENABLE_SANITIZERS "Enable sanitizers" OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

if(ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

add_library(skynet STATIC
    src/core/task.cpp
    src/core/executor.cpp
    src/core/timer_wheel.cpp
    src/core/thread_pool.cpp
    src/net/io_context.cpp
    src/net/socket.cpp
    src/net/acceptor.cpp
    src/net/tcp_stream.cpp
    src/net/resolver.cpp
    src/http/parser.cpp
    src/http/response.cpp
    src/http/router.cpp
    src/http/headers.cpp
    src/proxy/upstream.cpp
    src/proxy/load_balancer.cpp
    src/proxy/health_check.cpp
    src/proxy/connection_pool.cpp
    src/proxy/middleware.cpp
    src/config/config.cpp
)

find_package(Threads REQUIRED)

target_include_directories(skynet PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(skynet PUBLIC Threads::Threads)
target_compile_features(skynet PUBLIC cxx_std_20)

option(SKYNET_BUILD_GATEWAY "Build skynet_gateway executable" ON)
option(SKYNET_BUILD_TESTS "Build SkyNet unit tests" ${ENABLE_TESTS})
option(SKYNET_BUILD_EXAMPLES "Build SkyNet examples" OFF)

if(SKYNET_BUILD_GATEWAY AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gateway/main.cpp")
    add_executable(skynet_gateway gateway/main.cpp)
    target_link_libraries(skynet_gateway PRIVATE skynet)
endif()

if(SKYNET_BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

if(SKYNET_BUILD_TESTS)
    if(NOT TARGET GTest::gtest_main)
        include(FetchContent)
        FetchContent_Declare(googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
    enable_testing()
    add_subdirectory(tests)
endif()

if(ENABLE_BENCHMARKS AND CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    add_subdirectory(benches)
endif()
