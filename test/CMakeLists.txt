enable_testing()
include(GoogleTest)

set(TARGET net_ferry_test)

set(TEST_SRC
        common/config_test.cpp
        common/file_utils_test.cpp
        processor/inside_processor_test.cpp
        processor/outside_processor_test.cpp
        network/session_test.cpp
        network/http_client_test.cpp
        integration/integration_test.cpp
)

add_executable(${TARGET} ${TEST_SRC})

file(GLOB ABSL_STATIC_LIBS "${LIB_PREFIX_PATH}/lib/libabsl_*.a")

set(PROTOBUF_STATIC_LIBS
        ${LIB_PREFIX_PATH}/lib/libprotobuf.a
        ${ABSL_STATIC_LIBS}
        ${LIB_PREFIX_PATH}/lib/libz.dll.a
        ${LIB_PREFIX_PATH}/lib/libutf8_validity.a
        ${LIB_PREFIX_PATH}/lib/libdbghelp.a
)

target_link_libraries(${TARGET} PRIVATE
        GTest::gtest_main
        stupidcoder::net_ferry_core
        stupidcoder::net_ferry_proto
        "-Wl,--start-group"
        ${PROTOBUF_STATIC_LIBS}
        "-Wl,--end-group"
)

target_include_directories(${TARGET} PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/proto
        ${ASIO_INCLUDE_DIR}
)

gtest_discover_tests(${TARGET}
        WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
if(WIN32)
    target_link_libraries(net_ferry_test PRIVATE ws2_32 mswsock)
endif()