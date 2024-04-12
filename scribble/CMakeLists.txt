add_subdirectory("src")
add_subdirectory("rt")
add_subdirectory("test")

install(TARGETS scribblert testlib sv_test type_test
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
install(DIRECTORY share DESTINATION .)
