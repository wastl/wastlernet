INCLUDE(FindProtobuf)

# glog is provided at the root via FetchContent (v0.7.1)
if (NOT TARGET glog::glog)
    message(FATAL_ERROR "glog::glog target not available. Ensure the root CMakeLists fetches glog 0.7.1.")
endif()
find_package(absl REQUIRED)
FIND_PACKAGE(Protobuf REQUIRED)
INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)
PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HEADER shelly.proto)

add_compile_definitions(-DGLOG_USE_GLOG_EXPORT)
ADD_LIBRARY(shelly_client
        ${PROTO_HEADER} ${PROTO_SRC}
        shelly_module.cpp shelly_module.h
        shelly_timescaledb.cpp shelly_timescaledb.h)
## Find and link libmosquitto (MQTT)
find_library(MOSQUITTO_LIBRARY NAMES mosquitto)
if (NOT MOSQUITTO_LIBRARY)
    message(FATAL_ERROR "Could not find libmosquitto. Please install 'libmosquitto-dev' (Debian/Ubuntu) or equivalent.")
endif()

target_link_libraries(shelly_client PUBLIC
        gumbo_query
        glog::glog
        pqxx ${PostgreSQL_LIBRARIES}
        cpprestsdk::cpprest
        ${MOSQUITTO_LIBRARY}
        # Prometheus headers are included via base/metrics.h; link to ensure include dirs and symbols are available
        prometheus-cpp::core prometheus-cpp::pull
)
ADD_DEPENDENCIES(shelly_client config)

# Standalone debug client that prints received Shelly MQTT messages to stdout
add_executable(shelly_debug shelly_debug.cpp)
target_link_libraries(shelly_debug PRIVATE
        shelly_client
        config
        glog::glog
        cpprestsdk::cpprest
        ${Protobuf_LIBRARIES}
        absl::strings absl::status absl::statusor absl::str_format_internal absl::throw_delegate absl::hash absl::city absl::raw_hash_set absl::synchronization
)