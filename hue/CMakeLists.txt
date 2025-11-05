INCLUDE(FindProtobuf)
FIND_PACKAGE(Protobuf REQUIRED)

# glog is provided at the root via FetchContent (v0.7.1)
if (NOT TARGET glog::glog)
    message(FATAL_ERROR "glog::glog target not available. Ensure the root CMakeLists fetches glog 0.7.1.")
endif()
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)
PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HEADER hue.proto)

add_compile_definitions(-DGLOG_USE_GLOG_EXPORT)
ADD_LIBRARY(hue_client hue_client.cpp hue_client.h ${PROTO_HEADER} ${PROTO_SRC})
ADD_DEPENDENCIES(hue_client config)



