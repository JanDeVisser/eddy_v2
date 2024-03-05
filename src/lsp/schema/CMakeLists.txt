include_directories(.)

add_executable(
        ts
        ts.c
)

target_link_libraries(ts base template)
target_compile_definitions(ts PUBLIC TS_PARSER)

install(TARGETS ts
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
