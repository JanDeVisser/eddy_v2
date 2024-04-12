include_directories("${PROJECT_SOURCE_DIR}/base")

add_library(
        base
        STATIC
        da.c
        errorcode.c
        error_or.c
        fmt.c
        fn.c
        fs.c
        hash.c
        hm.c
        http.c
        integer.c
        io.c
        json.c
        lexer.c
        log.c
        mem.c
        mutex.c
        options.c
        pipe.c
        process.c
        resolve.c
        sb.c
        sl.c
        ss.c
        sv.c
        threadonce.h
        token.c
        xml.c
)

add_executable(
        sv_test
        sv.c
)

target_link_libraries(sv_test base)
target_compile_definitions(sv_test PUBLIC SV_TEST)

add_executable(
        fmt_test
        fmt.c
)

target_link_libraries(fmt_test base)
target_compile_definitions(fmt_test PUBLIC FMT_TEST)

add_executable(
        http_test
        http.c
)

target_link_libraries(http_test base)
target_compile_definitions(http_test PUBLIC HTTP_TEST)

add_executable(
        json_test
        json.c
)

target_link_libraries(json_test base)
target_compile_definitions(json_test PUBLIC JSON_TEST)

add_executable(
        json_format
        json.c
)

target_link_libraries(json_format base)
target_compile_definitions(json_format PUBLIC JSON_FORMAT)

add_executable(
        xml_format
        xml.c
)

target_link_libraries(xml_format base)
target_compile_definitions(xml_format PUBLIC XML_FORMAT)

add_subdirectory("test")


install(TARGETS json_format xml_format
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
