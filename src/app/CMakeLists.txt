add_executable(
        eddy
        buffer.c
        eddy.c
        editor.c
        main.c
        palette.c
        widget.c
)

target_include_directories(eddy PRIVATE ${raylib_INCLUDE_DIRS})
target_link_libraries(eddy PRIVATE base lang lsp ${raylib_LIBRARIES})

install(TARGETS eddy
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
