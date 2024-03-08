include_directories(.)

add_executable(
        ts
        generate.c
        ts.c
        parse.c
        serialize.c
        tostring.c
)

target_link_libraries(ts template base)
target_compile_definitions(ts PUBLIC TS_PARSER)

install(TARGETS ts
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
