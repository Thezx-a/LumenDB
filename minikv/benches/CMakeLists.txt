add_executable(bench_skip_list bench_skip_list.cpp)
target_link_libraries(bench_skip_list PRIVATE minikv)

add_executable(bench_db bench_db.cpp)
target_link_libraries(bench_db PRIVATE minikv)
