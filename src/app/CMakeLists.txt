add_executable(
        eddy
        buffer.c
        c.c
        cmake.c
        eddy.c
        editor.c
        listbox.c
        main.c
        minibuffer.c
        palette.c
        scribble.c
        widget.c
)

target_include_directories(eddy PRIVATE ${raylib_INCLUDE_DIRS})
target_link_libraries(eddy PRIVATE lsp ${raylib_LIBRARIES})

install(TARGETS eddy
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
