include_directories(.)

add_library(
        lsp
        STATIC
        lsp.c
)

target_include_directories(lsp PRIVATE ${raylib_INCLUDE_DIRS})
target_link_libraries(lsp base ${raylib_LIBRARIES})
