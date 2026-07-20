add_executable(lumendb_tests
    test_distance.cpp
    test_hnsw.cpp
    test_collection.cpp
    test_filter.cpp
    test_collection_filter.cpp
    test_quantize.cpp
)
target_link_libraries(lumendb_tests PRIVATE lumendb GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(lumendb_tests)
