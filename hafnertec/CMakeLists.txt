INCLUDE(FindProtobuf)

# glog is provided at the root via FetchContent (v0.7.1)
if (NOT TARGET glog::glog)
    message(FATAL_ERROR "glog::glog target not available. Ensure the root CMakeLists fetches glog 0.7.1.")
endif()
FIND_PACKAGE(Protobuf REQUIRED)
INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)
PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HEADER hafnertec.proto)

add_compile_definitions(-DGLOG_USE_GLOG_EXPORT)
ADD_LIBRARY(hafnertec_client hafnertec_client.cpp hafnertec_client.h ${PROTO_HEADER} ${PROTO_SRC} hafnertec_timescaledb.cpp hafnertec_timescaledb.h hafnertec_module.cpp hafnertec_module.h)

add_executable(hafnertec_modbus_debug hafnertec_modbus_debug.cpp)
target_link_libraries(hafnertec_modbus_debug PUBLIC pthread glog::glog absl_strings ${MODBUS_LIBRARY}  )
ADD_DEPENDENCIES(hafnertec_client config)