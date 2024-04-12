include_directories(.)

add_library(
        scb_model
        STATIC
        model/error.c
        model/op.c
        model/syntaxnode.c
)

target_link_libraries(scb_model base)

add_library(
        scb_impl
        STATIC
        backend.c
        binder.c
        datum.c
        execute.c
        frontend.c
        graph.c
        intermediate.c
        ir.c
        native.c
        parser.c
        type.c
)

target_link_libraries(scb_impl scb_model)

add_executable(
        scribble
        engine.h
        frontend_main.c
)

target_link_libraries(scribble scb_impl scb_arm64 scribblert)

add_executable(
        scribble-backend
        backend_main.c
        engine.h
)

target_link_libraries(scribble-backend scb_impl scb_arm64 scribblert)

add_executable(
        scribble-threaded
        engine.h
        frontend_main.c
)

target_link_libraries(scribble-threaded scb_impl scb_arm64 scribblert)
target_compile_definitions(scribble-threaded PUBLIC SCRIBBLE_THREADED_BACKEND)

add_library(
        testlib
        SHARED
        testlib.c
)

add_executable(
        type_test
        type.c
)

target_link_libraries(type_test scb_impl)
target_compile_definitions(type_test PUBLIC TYPE_TEST)

install(TARGETS scribble scribble-backend scb_model scb_impl testlib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
