add_executable(deepvector_tests
    test_distance.cpp
    test_hnsw.cpp
    test_collection.cpp
    test_filter.cpp
    test_collection_filter.cpp
    test_quantize.cpp
)
target_link_libraries(deepvector_tests PRIVATE deepvector GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(deepvector_tests)
