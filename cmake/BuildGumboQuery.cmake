# Fetch and build gumbo-query if it is not available on the system
# This script creates a target named `gumbo_query` and exposes the include directory so
# headers can be included as <gumbo-query/Document.h>.

include(FetchContent)

# Try to detect an existing target first (e.g., provided by the system)
if (TARGET gumbo_query)
    message(STATUS "gumbo_query library available (pre-existing target).")
    return()
endif()

# Fetch the gumbo-query sources
# Using a well-maintained fork that contains the original sources with CMake-friendly layout is not guaranteed,
# so we fetch the canonical repository and build the library ourselves.
# Repository: https://github.com/lazytiger/gumbo-query
FetchContent_Declare(
    gumbo_query_src
    GIT_REPOSITORY https://github.com/lazytiger/gumbo-query.git
    GIT_TAG        master
)

FetchContent_MakeAvailable(gumbo_query_src)

# Create the library from fetched sources (the repo doesn't always provide CMake targets)
# Source files are under src/; headers are included as <gumbo-query/...>
file(GLOB GUMBO_QUERY_SOURCES
    "${gumbo_query_src_SOURCE_DIR}/src/*.cpp"
    "${gumbo_query_src_SOURCE_DIR}/src/*.cc"
)

if (GUMBO_QUERY_SOURCES STREQUAL "")
    message(FATAL_ERROR "Failed to locate gumbo-query source files in ${gumbo_query_src_SOURCE_DIR}/src")
endif()

add_library(gumbo_query ${GUMBO_QUERY_SOURCES})
add_library(gumbo_query::gumbo_query ALIAS gumbo_query)

# Provide include directories.
# 1) Direct repo include for internal includes like "Document.h"
# 2) Synthetic include root that contains a folder named "gumbo-query" so
#    project sources can include headers as <gumbo-query/Document.h>
#    We copy the .h files from the repo's src/ into ${CMAKE_BINARY_DIR}/third_party/gumbo-query
#    and then add ${CMAKE_BINARY_DIR}/third_party to PUBLIC include dirs.

# Direct include to support internal relative includes
target_include_directories(gumbo_query
    PUBLIC
        "${gumbo_query_src_SOURCE_DIR}/src"
)

# Create synthetic include tree for <gumbo-query/…>
set(GUMBO_QUERY_SYNTH_INC_ROOT "${CMAKE_BINARY_DIR}/third_party")
set(GUMBO_QUERY_SYNTH_DIR "${GUMBO_QUERY_SYNTH_INC_ROOT}/gumbo-query")
file(MAKE_DIRECTORY "${GUMBO_QUERY_SYNTH_DIR}")
file(GLOB GUMBO_QUERY_HEADERS "${gumbo_query_src_SOURCE_DIR}/src/*.h")
if (GUMBO_QUERY_HEADERS)
    file(COPY ${GUMBO_QUERY_HEADERS} DESTINATION "${GUMBO_QUERY_SYNTH_DIR}")
endif()

# Expose the synthetic include root so <gumbo-query/Document.h> resolves
target_include_directories(gumbo_query PUBLIC "${GUMBO_QUERY_SYNTH_INC_ROOT}")

# gumbo-query depends on gumbo (C Gumbo HTML5 parser). Make sure it's found and linked.
# The root CMake includes FindGumbo.cmake which defines target `gumbo` via variables.
# We attempt to link against a `gumbo` target if available, otherwise use the variable.
if (TARGET gumbo)
    target_link_libraries(gumbo_query PUBLIC gumbo)
else()
    # Try to use variables provided by FindGumbo.cmake
    if (DEFINED Gumbo_LIBRARIES)
        target_link_libraries(gumbo_query PUBLIC ${Gumbo_LIBRARIES})
    else()
        message(FATAL_ERROR "Gumbo (HTML5 parser) not found. Ensure FindGumbo.cmake finds it or install libgumbo.")
    endif()
endif()

set_target_properties(gumbo_query PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
