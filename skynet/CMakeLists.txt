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

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    add_executable(skynet_gateway gateway/main.cpp)
    target_link_libraries(skynet_gateway PRIVATE skynet)

    add_subdirectory(examples)

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
        add_subdirectory(benches)
    endif()
endif()
