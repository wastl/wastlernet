INCLUDE(FetchContent)

# Try to find a pre-installed GTest first (Config or Module)
find_package(GTest QUIET)

if (NOT TARGET GTest::gtest)
    message(STATUS "GTest not found, fetching googletest with FetchContent...")
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
    )
    # Avoid overriding parent project settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    # Ensure standard imported target names exist
    if (TARGET gtest AND NOT TARGET GTest::gtest)
        add_library(GTest::gtest ALIAS gtest)
    endif()
    if (TARGET gtest_main AND NOT TARGET GTest::gtest_main)
        add_library(GTest::gtest_main ALIAS gtest_main)
    endif()
endif()
