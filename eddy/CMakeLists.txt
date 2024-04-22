set(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR} CACHE PATH "..." FORCE)

include_directories("src/app")

add_subdirectory(src/template)
add_subdirectory(src/app)

install(DIRECTORY share DESTINATION .)
