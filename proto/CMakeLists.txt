set(TARGET net_ferry_proto)
set(PROTO_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

add_library(${TARGET} OBJECT
        packet.proto
)

add_library(stupidcoder::net_ferry_proto ALIAS ${TARGET})

target_link_libraries(${TARGET} PUBLIC
        ${PROTOBUF_LIBRARIES}
)
target_include_directories(${TARGET} PUBLIC
        "$<BUILD_INTERFACE:${PROTO_BINARY_DIR}>"
)

protobuf_generate(
        TARGET ${TARGET}
        IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
        PROTOC_OUT_DIR "${PROTO_BINARY_DIR}"
)