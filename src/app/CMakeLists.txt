add_executable(
        eddy
        main.c
        widget.c
        buffer.c
        buffer.h
)

target_include_directories(eddy PRIVATE ${raylib_INCLUDE_DIRS})
target_link_libraries(eddy PRIVATE base ${raylib_LIBRARIES})

install(TARGETS eddy
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
