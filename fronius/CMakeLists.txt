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
ADD_LIBRARY(fronius_client ${PROTO_HEADER} ${PROTO_SRC} fronius_timescaledb.cpp fronius_timescaledb.h fronius_module.cpp fronius_module.h fronius_modbus.h)
TARGET_LINK_LIBRARIES(fronius_client PUBLIC absl_strings absl_status absl_throw_delegate glog::glog ${MODBUS_LIBRARY} )
ADD_DEPENDENCIES(fronius_client config)