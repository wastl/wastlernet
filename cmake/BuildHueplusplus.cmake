# Fetch and build hueplusplus if it is not available on the system
# This script ensures a target named `hueplusplus::hueplusplus` exists.

include(FetchContent)

# First, try to use an existing package/target if it is already available
find_package(hueplusplus QUIET CONFIG)
if (TARGET hueplusplus::hueplusplus)
    message(STATUS "hueplusplus available (pre-existing target hueplusplus::hueplusplus).")
    return()
endif()

# Sometimes distro packages expose targets without the namespace
if (TARGET hueplusplus)
    add_library(hueplusplus::hueplusplus ALIAS hueplusplus)
    message(STATUS "hueplusplus available (created alias hueplusplus::hueplusplus -> hueplusplus).")
    return()
endif()

# Fallback: FetchContent from upstream repository
message(STATUS "hueplusplus not found, fetching with FetchContent...")
FetchContent_Declare(
    hueplusplus_src
    GIT_REPOSITORY https://github.com/enwi/hueplusplus.git
    GIT_TAG        master
)

# Options to build the Linux Http Handler (uses libcurl). Keep defaults if not provided by parent.
set(BUILD_LINUX_HTTP_HANDLER ON CACHE BOOL "Build Linux HTTP handler for hueplusplus" FORCE)

# Make mbedtls (submodule) not treat warnings as errors under clang
set(MBEDTLS_FATAL_WARNINGS OFF CACHE BOOL "mbedtls: do not treat warnings as errors" FORCE)

FetchContent_MakeAvailable(hueplusplus_src)

# Upstream's CMake usually defines targets named `hueplusplusshared` and `hueplusplusstatic` and/or an exported namespace target.
# Normalize to hueplusplus::hueplusplus if possible; otherwise, consumers can link the concrete targets directly.
if (NOT TARGET hueplusplus::hueplusplus)
    if (TARGET hueplusplus)
        add_library(hueplusplus::hueplusplus ALIAS hueplusplus)
    elseif(TARGET hueplusplusshared)
        add_library(hueplusplus::hueplusplus ALIAS hueplusplusshared)
    elseif(TARGET hueplusplusstatic)
        add_library(hueplusplus::hueplusplus ALIAS hueplusplusstatic)
    endif()
endif()
