INCLUDE(FindProtobuf)
FIND_PACKAGE(Protobuf REQUIRED)

# Dependencies
find_package(absl REQUIRED)

# glog is provided at the root via FetchContent (v0.7.1)
if (NOT TARGET glog::glog)
    message(FATAL_ERROR "glog::glog target not available. Ensure the root CMakeLists fetches glog 0.7.1.")
endif()
# hueplusplus is provided via BuildHueplusplus in the root CMakeLists
if (NOT TARGET hueplusplus::hueplusplus)
    message(FATAL_ERROR "hueplusplus::hueplusplus target not available. Ensure BuildHueplusplus.cmake is included in the root CMakeLists.")
endif()

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)
PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HEADER hue.proto)

add_compile_definitions(-DGLOG_USE_GLOG_EXPORT)
ADD_LIBRARY(hue_client hue_client.cpp hue_client.h ${PROTO_HEADER} ${PROTO_SRC})
ADD_DEPENDENCIES(hue_client config)

target_link_libraries(hue_client
        PUBLIC
        hueplusplus::hueplusplus
        glog::glog
        absl::strings absl::status absl::hash absl::city absl::raw_hash_set
        ${Protobuf_LIBRARIES}
)



