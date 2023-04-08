INCLUDE(FindProtobuf)
FIND_PACKAGE(Protobuf REQUIRED)
INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/..)
PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HEADER weather.proto)

ADD_LIBRARY(weather_client weather_listener.cpp weather_listener.h ${PROTO_HEADER} ${PROTO_SRC})

# add_executable(weather_exporter main.cpp weather_listener.cpp weather_listener.h weather_influx.cpp weather_influx.h)
# target_link_libraries(weather_exporter PUBLIC cpprest pthread ssl crypto yaml-cpp glog)

