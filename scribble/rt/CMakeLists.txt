
include_directories(.)

enable_language(ASM-ATT)

add_library(
        scribblestart
        STATIC
        arch/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}/start.s
)

add_library(
        trampoline
        STATIC
        arch/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}/trampoline.s
)

add_library(
        scribblert
        SHARED
        arch/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}/alloc.s
        arch/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}/endln.s
        arch/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}/open.s
        arch/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}/puthex.s
        arch/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}/puti.s
        arch/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}/puts.s
        arch/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}/putln.s
        arch/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}/string_cmp.s
        arch/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}/string_concat.s
        arch/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}/string_eq.s
        arch/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}/strlen.s
        arch/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}/to_string.s
)

set_target_properties(scribblert PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(scribblert PRIVATE base)

install(TARGETS scribblert scribblestart
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
