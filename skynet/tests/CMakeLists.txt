add_executable(skynet_tests
    test_task.cpp
    test_parser.cpp
    test_router.cpp
    test_load_balancer.cpp
)
target_link_libraries(skynet_tests PRIVATE skynet GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(skynet_tests)
