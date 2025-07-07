cmake_minimum_required(VERSION 3.30)
project(wastlernet)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
enable_testing()

INCLUDE(FindGTest)
INCLUDE(FindPostgreSQL)
INCLUDE(FindGumbo)
INCLUDE(FindOpenSSL)

INCLUDE(GoogleTest)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(gflags REQUIRED)

find_package (glog 0.7.1 REQUIRED)
add_compile_definitions(-DGLOG_USE_GLOG_EXPORT)
if (TARGET glog::glog)
    message(STATUS "glog library found successfully.")
else()
    message(FATAL_ERROR "Could not find glog library. Ensure it is available on your system.")
endif()

find_package(Protobuf REQUIRED)
if (DEFINED Protobuf_FOUND)
    message(STATUS "Protobuf environment found successfully.")
else()
    message(FATAL_ERROR "Could not find Protobuf environment. Ensure it is available on your system.")
endif()

find_library(MODBUS_LIBRARY NAMES libmodbus modbus)
find_library(PQXX_LIBRARY NAMES libpqxx pqxx)

find_package(hueplusplus REQUIRED)
find_library(HUE_LIBRARY NAMES hueplusplusshared hueplusplusstatic)

find_package(absl REQUIRED)

find_package(cpprestsdk REQUIRED)
if (TARGET cpprestsdk::cpprest)
    message(STATUS "CppRestSDK (Casablanca) library found successfully.")
else()
    message(FATAL_ERROR "Could not find CppRestSDK (Casablanca) library. Ensure it is available on your system.")
endif()

# look for threading implementation
find_package(Threads REQUIRED)
if (TARGET Threads::Threads)
    message(STATUS "pthread library found successfully.")
else()
    message(FATAL_ERROR "Could not find pthread library. Ensure it is available on your system.")
endif()

if (TARGET OpenSSL::Crypto)
    message(STATUS "crypto library found successfully.")
else()
    message(FATAL_ERROR "Could not find crypto library. Ensure it is available on your system.")
endif()


find_package(GTest REQUIRED)

ADD_SUBDIRECTORY(config)
ADD_SUBDIRECTORY(hafnertec)
ADD_SUBDIRECTORY(hue)
ADD_SUBDIRECTORY(senec)
ADD_SUBDIRECTORY(weather)
ADD_SUBDIRECTORY(solvis)
ADD_SUBDIRECTORY(fronius)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

SET(ABSL_LIBRARIES absl::strings absl::status absl::statusor absl::str_format_internal absl::throw_delegate absl::hash absl::city
        absl::raw_hash_set absl::synchronization)
SET(HTTP_SRC ../include/http_connection.h ../include/http_connection.cpp)

ADD_EXECUTABLE(wastlernet
        main.cpp
        include/module.h include/updater.h
        include/utility.h include/utility.cpp
        include/modbus_connection.h include/modbus_connection.cpp
        include/http_connection.h include/http_connection.cpp
)
TARGET_LINK_LIBRARIES(wastlernet
        config
        hafnertec_client solvis_client senec_client weather_client fronius_client
        cpprestsdk::cpprest Threads::Threads gumbo gumbo_query
        ${ABSL_LIBRARIES}
        pqxx
        OpenSSL::Crypto
        glog::glog
        ${PostgreSQL_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${Protobuf_LIBRARIES}
        ${MODBUS_LIBRARY})

ADD_EXECUTABLE(modbus_test
        include/modbus_connection_test.cpp
        include/modbus_connection.h include/modbus_connection.cpp
        include/utility.h include/utility.cpp
)
TARGET_LINK_LIBRARIES(modbus_test
        GTest::gtest GTest::gtest_main
        Threads::Threads
        glog::glog
        ${ABSL_LIBRARIES}
        ${MODBUS_LIBRARY}
)

ADD_EXECUTABLE(http_test
        include/http_connection_test.cpp
        include/http_connection.h include/http_connection.cpp
        include/utility.h include/utility.cpp
)
TARGET_LINK_LIBRARIES(http_test
        GTest::gtest GTest::gtest_main
        Threads::Threads
        glog::glog
        ${ABSL_LIBRARIES}
        OpenSSL::Crypto
        cpprestsdk::cpprest
)

ADD_EXECUTABLE(hafnertec_client_test hafnertec/hafnertec_client_test.cpp ${HTTP_SRC} ${PROTO_SRC})
TARGET_LINK_LIBRARIES(hafnertec_client_test
        hafnertec_client
        GTest::gtest GTest::gtest_main
        Threads::Threads
        gumbo gumbo_query
        glog::glog
        ${ABSL_LIBRARIES}
        OpenSSL::Crypto
        ${Protobuf_LIBRARIES}
        cpprestsdk::cpprest
)

gtest_discover_tests(hafnertec_client_test)
gtest_discover_tests(modbus_test)
gtest_discover_tests(http_test)

#ADD_EXECUTABLE(hue_debug hue/hue_debug.cpp)
#TARGET_LINK_LIBRARIES(hue_debug PUBLIC hue_client ${HUE_LIBRARY} ${GLOG_LIBRARY} ${Protobuf_LIBRARIES} config glog::glog crypto absl_strings absl_status absl_hash absl_city absl_raw_hash_set absl_throw_delegate)