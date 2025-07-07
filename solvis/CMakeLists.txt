INCLUDE(FindProtobuf)
FIND_PACKAGE(Protobuf REQUIRED)
FIND_PACKAGE (glog 0.7.1 REQUIRED)
INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)
PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HEADER solvis.proto)


find_library(MODBUS_LIBRARY NAMES libmodbus modbus)

add_compile_definitions(-DGLOG_USE_GLOG_EXPORT)
ADD_LIBRARY(solvis_client ${PROTO_HEADER} ${PROTO_SRC} solvis_timescaledb.cpp solvis_timescaledb.h solvis_module.cpp solvis_module.h solvis_updater.cpp solvis_updater.h solvis_modbus.h)
TARGET_LINK_LIBRARIES(solvis_client PUBLIC glog absl_strings absl_status absl_throw_delegate glog::glog ${MODBUS_LIBRARY} )