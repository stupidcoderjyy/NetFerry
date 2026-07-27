set(TARGET net_ferry_core)

add_library(${TARGET} STATIC
        common/config.cpp
        common/file_utils.cpp
        network/http_client.cpp
        network/session.cpp
        network/tcp_server.cpp
        processor/inside_processor.cpp
        processor/outside_processor.cpp
        watcher/directory_watcher.cpp
)

file(GLOB ABSL_STATIC_LIBS "${LIB_PREFIX_PATH}/lib/libabsl_*.a")

set(PROTOBUF_STATIC_LIBS
        ${LIB_PREFIX_PATH}/lib/libprotobuf.a
        ${ABSL_STATIC_LIBS}
        ${LIB_PREFIX_PATH}/lib/libz.dll.a
        ${LIB_PREFIX_PATH}/lib/libutf8_validity.a
        ${LIB_PREFIX_PATH}/lib/libdbghelp.a
)

target_link_libraries(${TARGET} PRIVATE
        stupidcoder::net_ferry_proto
        nlohmann_json::nlohmann_json
        "-Wl,--start-group"
        ${PROTOBUF_STATIC_LIBS}
        "-Wl,--end-group"
)

target_link_libraries(${TARGET} PUBLIC
        efsw
        spdlog::spdlog
        fmt::fmt
)

if(WIN32)
    target_link_libraries(${TARGET} PUBLIC ws2_32 mswsock)
endif()

add_library(stupidcoder::net_ferry_core ALIAS ${TARGET})

target_compile_definitions(${TARGET} PUBLIC SPDLOG_SHARED_LIB SPDLOG_COMPILED_LIB SPDLOG_FMT_EXTERNAL)
target_include_directories(${TARGET} PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${PROTOBUF_INCLUDE_DIRS}
        ${CMAKE_BINARY_DIR}/proto
        ${ASIO_INCLUDE_DIR}
)