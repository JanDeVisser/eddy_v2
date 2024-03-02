include_directories(.)

add_library(
        template
        STATIC
        template.c
        parser.c
        render.c
)

target_link_libraries(template base)

add_executable(
        render
        template.c
)

target_link_libraries(render template)
target_compile_definitions(render PUBLIC TEMPLATE_RENDER)

install(TARGETS render template
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
