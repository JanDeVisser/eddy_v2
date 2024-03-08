include_directories(.)

add_library(
        template
        STATIC
        lexer.c
        parser.c
        render.c
        serialize.c
        template.c
)

install(TARGETS template
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

add_subdirectory(test)
