include_directories(.)

add_library(
        lang
        STATIC
        ts.c
)

add_executable(
        ts
        ts.c
)

target_link_libraries(ts base)
target_compile_definitions(ts PUBLIC TS_PARSER)

install(TARGETS ts
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
