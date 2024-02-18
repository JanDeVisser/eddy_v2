add_executable(
        eddy
        buffer.c
        c.c
        eddy.c
        editor.c
        main.c
        palette.c
        scribble.c
        widget.c
)

target_include_directories(eddy PRIVATE ${raylib_INCLUDE_DIRS})
target_link_libraries(eddy PRIVATE base lsp ${raylib_LIBRARIES})

install(TARGETS eddy
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
