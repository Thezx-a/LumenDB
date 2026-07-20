include(FetchContent)
FetchContent_Declare(pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v2.11.1
)
FetchContent_MakeAvailable(pybind11)

pybind11_add_module(_lumendb bindings.cpp)
target_link_libraries(_lumendb PRIVATE lumendb)
target_include_directories(_lumendb PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(_lumendb PRIVATE VERSION_INFO="0.1.0")
