include_directories(.)

add_executable(
        template_test
        template_test.c
)

target_link_libraries(template_test template base)

install(TARGETS template_test
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
