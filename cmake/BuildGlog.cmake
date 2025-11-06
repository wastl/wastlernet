INCLUDE(FetchContent)

find_package(glog 0.7.1 QUIET CONFIG)
if (NOT TARGET glog::glog)
    message(STATUS "glog 0.7.1 not found, fetching with FetchContent...")
    include(FetchContent)
    FetchContent_Declare(
            glog
            GIT_REPOSITORY https://github.com/google/glog.git
            GIT_TAG v0.7.1
    )
    # Ensure glog uses external gflags if available
    set(WITH_GFLAGS ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glog)
endif()
