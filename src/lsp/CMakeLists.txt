include_directories(.)

add_library(
        lsp
        STATIC
        lsp.c
        lsp_base.c
        base.c
        initialize.c
        semantictokens.c
        synchronization.c
)

target_include_directories(lsp PRIVATE ${raylib_INCLUDE_DIRS})
