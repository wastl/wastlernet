INCLUDE(FindProtobuf)
FIND_PACKAGE(Protobuf REQUIRED)

# glog is provided at the root via FetchContent (v0.7.1)
if (NOT TARGET glog::glog)
    message(FATAL_ERROR "glog::glog target not available. Ensure the root CMakeLists fetches glog 0.7.1.")
endif()
INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)
PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HEADER fronius.proto)


find_library(MODBUS_LIBRARY NAMES libmodbus modbus)

add_compile_definitions(-DGLOG_USE_GLOG_EXPORT)
ADD_LIBRARY(fronius_client ${PROTO_HEADER} ${PROTO_SRC}
        fronius_timescaledb.cpp fronius_timescaledb.h
        fronius_client.cpp fronius_client.h
        fronius_module.cpp fronius_module.h
        ${CMAKE_SOURCE_DIR}/base/http_connection.h ${CMAKE_SOURCE_DIR}/base/http_connection.cpp)
TARGET_LINK_LIBRARIES(fronius_client PUBLIC absl_strings absl_status absl_throw_delegate glog::glog cpprestsdk::cpprest )
ADD_DEPENDENCIES(fronius_client config)

# Simple CLI to test Fronius Solar API JSON client against a live server
add_executable(fronius_client_debug
        fronius_client_debug.cpp
        ${CMAKE_SOURCE_DIR}/base/metrics.h ${CMAKE_SOURCE_DIR}/base/metrics.cpp
        ${CMAKE_SOURCE_DIR}/base/utility.h ${CMAKE_SOURCE_DIR}/base/utility.cpp
)
# Prefer imported gflags target when available; fall back to variable
set(FRONIUS_GFLAGS_LIB gflags)
if (TARGET gflags::gflags)
    set(FRONIUS_GFLAGS_LIB gflags::gflags)
endif()

target_link_libraries(fronius_client_debug
        PRIVATE
        fronius_client
        ${FRONIUS_GFLAGS_LIB}
        glog::glog
        ${Protobuf_LIBRARIES}
        ${ABSL_LIBRARIES} absl::raw_logging_internal absl::str_format_internal absl::log_severity absl::synchronization absl::statusor absl::hash absl::city absl::raw_hash_set
        pqxx
        ${PostgreSQL_LIBRARIES}
        Threads::Threads
        prometheus-cpp::core prometheus-cpp::pull
        OpenSSL::Crypto
)
add_dependencies(fronius_client_debug fronius_client)