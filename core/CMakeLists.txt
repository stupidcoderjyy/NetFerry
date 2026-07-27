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

target_link_libraries(${TARGET} PRIVATE
        stupidcoder::net_ferry_proto
        nlohmann_json::nlohmann_json
        protobuf::libprotobuf
        absl::log
        absl::log_internal_check_op
        absl::hash
        absl::strings
        absl::base
)

target_link_libraries(${TARGET} PUBLIC
        efsw
)

if(WIN32)
    target_link_libraries(${TARGET} PUBLIC ws2_32 mswsock)
endif()

add_library(stupidcoder::net_ferry_core ALIAS ${TARGET})

target_include_directories(${TARGET} PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${PROTOBUF_INCLUDE_DIRS}
        ${CMAKE_BINARY_DIR}/proto
        ${ASIO_INCLUDE_DIR}
)