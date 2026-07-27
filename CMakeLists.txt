cmake_minimum_required(VERSION 4.2)
project(NetFerry)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG  "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_PROJECT_VERSION 1.0)

set(LIB_PREFIX_PATH "C:/msys64/mingw64")
list(PREPEND CMAKE_PREFIX_PATH ${LIB_PREFIX_PATH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(ASIO_INCLUDE_DIR ${LIB_PREFIX_PATH}/include)

find_package(nlohmann_json REQUIRED)
find_package(Protobuf REQUIRED)
find_package(absl REQUIRED)

set(PROTOBUF_LIBRARIES
        protobuf::libprotobuf
        absl::log
        absl::hash
        absl::strings
        absl::base
        absl::status
        absl::statusor
        absl::synchronization
        absl::time
        absl::log_internal_check_op
)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        efsw
        stupidcoder::net_ferry_core
        stupidcoder::net_ferry_proto
        ${PROTOBUF_LIBRARIES}
)

add_subdirectory(proto)
add_subdirectory(thirdparty)
add_subdirectory(core)
add_subdirectory(test)

# Code formatting
find_program(CLANG_FORMAT
        NAMES clang-format
        PATHS ${LIB_PREFIX_PATH}/bin
        NO_DEFAULT_PATH
)
if(NOT CLANG_FORMAT)
    find_program(CLANG_FORMAT NAMES clang-format)
endif()

if(CLANG_FORMAT)
    file(GLOB_RECURSE ALL_SOURCE_FILES
            "${PROJECT_SOURCE_DIR}/core/*.cpp"
            "${PROJECT_SOURCE_DIR}/core/*.h"
    )
    list(APPEND ALL_SOURCE_FILES
            main.cpp
    )
    add_custom_target(format
            COMMAND ${CLANG_FORMAT} -i -style=file ${ALL_SOURCE_FILES}
            COMMENT "Running clang-format on all source files."
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
    add_dependencies(${PROJECT_NAME} format)

    message(STATUS "clang-format found at ${CLANG_FORMAT}")
else()
    message(WARNING "clang-format not found. The 'format' target will not be available.")
endif()