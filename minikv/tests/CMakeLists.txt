add_executable(minikv_tests
    test_skip_list.cpp
    test_bloom_filter.cpp
    test_slice.cpp
    test_coding.cpp
    test_lru_cache.cpp
    test_wal.cpp
    test_memtable.cpp
)
target_link_libraries(minikv_tests PRIVATE minikv GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(minikv_tests)
