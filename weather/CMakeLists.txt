INCLUDE(FindProtobuf)
FIND_PACKAGE (glog 0.7.1 REQUIRED)
FIND_PACKAGE(Protobuf REQUIRED)
INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)
PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HEADER weather.proto)

add_compile_definitions(-DGLOG_USE_GLOG_EXPORT)
ADD_LIBRARY(weather_client weather_listener.cpp weather_listener.h ${PROTO_HEADER} ${PROTO_SRC} weather_timescaledb.cpp weather_timescaledb.h weather_module.cpp weather_module.h)

