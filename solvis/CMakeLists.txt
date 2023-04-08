INCLUDE(FindProtobuf)
FIND_PACKAGE(Protobuf REQUIRED)
INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)
PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HEADER solvis.proto)


find_library(MODBUS_LIBRARY NAMES libmodbus modbus)
find_library(GLOG_LIBRARY NAMES libglog glog)

ADD_LIBRARY(solvis_client solvis_client.cpp solvis_client.h ${PROTO_HEADER} ${PROTO_SRC} solvis_timescaledb.cpp solvis_timescaledb.h solvis_module.cpp solvis_module.h)
TARGET_LINK_LIBRARIES(solvis_client PUBLIC glog absl_strings absl_status absl_throw_delegate ${MODBUS_LIBRARY} ${GLOG_LIBRARY} )

ADD_EXECUTABLE(solvis_debug solvis_debug.cpp)
TARGET_LINK_LIBRARIES(solvis_debug PUBLIC solvis_client ${MODBUS_LIBRARY} ${GLOG_LIBRARY} ${Protobuf_LIBRARIES} glog absl_strings absl_status absl_throw_delegate)

#add_executable(senec_exporter main.cpp senec_client.cpp senec_client.h senec_influx.cpp senec_influx.h)
#target_link_libraries(senec_exporter PUBLIC cpprest pthread ssl crypto yaml-cpp glog)
