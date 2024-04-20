find_package(Freetype)

add_executable(
        eddy
        app.c
        buffer.c
        c.c
        cmake.c
        eddy.c
        editor.c
        listbox.c
        main.c
        minibuffer.c
        mode.c
        scribble.c
        theme.c
        widget.c
)

target_include_directories(eddy PRIVATE ${raylib_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(eddy PRIVATE lsp scb_impl scb_arm64 scribblert ${raylib_LIBRARIES} ${FREETYPE_LIBRARIES})

install(TARGETS eddy
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
