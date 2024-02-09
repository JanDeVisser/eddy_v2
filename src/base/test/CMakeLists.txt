include_directories(.)

add_executable(
        http_server
        http_server.c
)

add_executable(
        http_client
        http_client.c
)

add_executable(
        clang-format-test
        clang-format.c
)

target_link_libraries(http_server base)
target_link_libraries(http_client base)
target_link_libraries(clang-format-test base)

install(TARGETS http_server http_client clang-format-test
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
