add_executable(echo_server echo_server.cpp)
target_link_libraries(echo_server PRIVATE skynet)

add_executable(http_server http_server.cpp)
target_link_libraries(http_server PRIVATE skynet)
