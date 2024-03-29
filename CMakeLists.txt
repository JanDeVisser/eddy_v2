cmake_minimum_required(VERSION 3.27)
project(eddy C)

set(CMAKE_C_STANDARD 23)

# On MacOS the brew-distributed raylib has an annoying bug which causes a
# +/- 1 second delay the first time IsKeyPressed is supposed to trigger.
# Using a built-from-source raylib with PLATFORM=SDL avoids this problem.
if(DEFINED RAYLIB)
  set(raylib_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/${RAYLIB}")
  set(raylib_LIBRARIES "-framework IOKit -framework Cocoa -framework OpenGL -L${PROJECT_SOURCE_DIR}/${RAYLIB} -lraylib -lSDL2")
else()
  set(raylib_VERBOSE ON)
  include(cmake/raylib-config.cmake)
endif()

set(eddy_NAME Eddy)
set(eddy_VERSION_MAJOR 1)
set(eddy_VERSION_MINOR 0)

set(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR} CACHE PATH "..." FORCE)

set(eddy_DIR ${CMAKE_INSTALL_PREFIX})
set(eddy_DATADIR ${CMAKE_INSTALL_PREFIX}/share)

include_directories("src" "src/base" "src/app" "${PROJECT_BINARY_DIR}")

include(CheckCSourceCompiles)

check_c_source_compiles("
   [[noreturn]] void testFunc()
   {}

   int main(void)
   {
     return 0;
   }
" HAVE_C23_ATTRIBUTE_NORETURN
)

check_c_source_compiles("
   __attribute__((noreturn)) void testFunc()
   {}

   int main(void)
   {
     return 0;
   }
" HAVE_LEGACY_ATTRIBUTE_NORETURN
)

check_c_source_compiles("
   __attribute__((format(printf,1,2))) void testFunc(char *fmt, ...)
   {
   }

   int main(void)
   {
     return 0;
   }
" HAVE_ATTRIBUTE_FORMAT
)

include(CheckStructHasMember)
check_struct_has_member("struct dirent" d_namlen dirent.h HAVE_DIRENT_D_NAMLEN LANGUAGE C)

configure_file(
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
)

add_subdirectory(src/base)
add_subdirectory(src/template)
add_subdirectory(src/lsp)
add_subdirectory(src/app)

install(DIRECTORY share DESTINATION .)
