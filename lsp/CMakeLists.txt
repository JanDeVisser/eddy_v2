include_directories(.)

add_library(
        lsp
        STATIC
        lsp.c
)

target_link_libraries(lsp PRIVATE lspschema)
target_include_directories(lsp PRIVATE ${raylib_INCLUDE_DIRS})
add_subdirectory(schema)
add_subdirectory(ts)
