set(TARGET net_ferry_core)

add_library(${TARGET} STATIC
        example.cpp
)

target_link_libraries(${TARGET} PRIVATE
        stupidcoder::net_ferry_proto
        nlohmann_json::nlohmann_json
        ${PROTOBUF_LIBRARIES}
)
add_library(stupidcoder::net_ferry_core ALIAS ${TARGET})
target_include_directories(${TARGET} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROTOBUF_INCLUDE_DIRS}
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        ${ASIO_INCLUDE_DIR}
)