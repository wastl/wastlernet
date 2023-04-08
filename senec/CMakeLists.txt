INCLUDE(FindProtobuf)
FIND_PACKAGE(Protobuf REQUIRED)
INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)
PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HEADER senec.proto)

ADD_LIBRARY(senec_client
        senec_client.cpp senec_client.h
        senec_timescaledb.cpp senec_timescaledb.h
        senec_module.cpp senec_module.h
        ${PROTO_HEADER} ${PROTO_SRC} )
