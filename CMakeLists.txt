cmake_minimum_required(VERSION 3.30)
project(wastlernet)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
enable_testing()

INCLUDE(BuildGTest)
INCLUDE(FindPostgreSQL)
INCLUDE(FindGumbo)
INCLUDE(FindOpenSSL)
INCLUDE(BuildGlog)
INCLUDE(BuildGumboQuery)
INCLUDE(BuildHueplusplus)

INCLUDE(GoogleTest)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(gflags REQUIRED)

add_compile_definitions(-DGLOG_USE_GLOG_EXPORT)
if (TARGET glog::glog)
    message(STATUS "glog library available.")
else()
    message(FATAL_ERROR "Could not obtain glog library (even after FetchContent).")
endif()

find_package(Protobuf REQUIRED)
if (DEFINED Protobuf_FOUND)
    message(STATUS "Protobuf environment found successfully.")
else()
    message(FATAL_ERROR "Could not find Protobuf environment. Ensure it is available on your system.")
endif()

find_library(MODBUS_LIBRARY NAMES libmodbus modbus)
find_library(PQXX_LIBRARY NAMES libpqxx pqxx)


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



find_package(prometheus-cpp REQUIRED)
if (TARGET prometheus-cpp::core)
    message(STATUS "Prometheus C++ library found successfully.")
else()
    message(FATAL_ERROR "Could not find Prometheus C++ library. Ensure it is available on your system.")
endif()
#target_link_libraries(your_target_name PRIVATE prometheus-cpp::core prometheus-cpp::pull)

ADD_SUBDIRECTORY(config)
ADD_SUBDIRECTORY(hafnertec)
ADD_SUBDIRECTORY(hue)
ADD_SUBDIRECTORY(senec)
ADD_SUBDIRECTORY(weather)
ADD_SUBDIRECTORY(solvis)
ADD_SUBDIRECTORY(fronius)
ADD_SUBDIRECTORY(shelly)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})


SET(ABSL_LIBRARIES absl::strings absl::status absl::statusor absl::str_format_internal absl::throw_delegate absl::hash absl::city
        absl::raw_hash_set absl::synchronization)
SET(HTTP_SRC ../base/http_connection.h ../base/http_connection.cpp)

ADD_EXECUTABLE(wastlernet
        main.cpp
        base/module.h base/updater.h
        base/utility.h base/utility.cpp
        base/metrics.h base/metrics.cpp
        base/modbus_connection.h base/modbus_connection.cpp
        base/http_connection.h base/http_connection.cpp
)
TARGET_LINK_LIBRARIES(wastlernet
        config
        hafnertec_client solvis_client senec_client weather_client fronius_client shelly_client
        cpprestsdk::cpprest Threads::Threads gumbo gumbo_query
        ${ABSL_LIBRARIES}
        pqxx
        OpenSSL::Crypto
        glog::glog
        prometheus-cpp::core prometheus-cpp::pull
        ${PostgreSQL_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${Protobuf_LIBRARIES}
        ${MODBUS_LIBRARY})

ADD_EXECUTABLE(modbus_test
        base/modbus_connection_test.cpp
        base/modbus_connection.h base/modbus_connection.cpp
        base/utility.h base/utility.cpp
        base/metrics.h base/metrics.cpp
)
TARGET_LINK_LIBRARIES(modbus_test
        GTest::gtest GTest::gtest_main
        Threads::Threads
        glog::glog
        prometheus-cpp::core prometheus-cpp::pull
        ${ABSL_LIBRARIES}
        ${MODBUS_LIBRARY}
)

ADD_EXECUTABLE(http_test
        base/http_connection_test.cpp
        base/http_connection.h base/http_connection.cpp
        base/utility.h base/utility.cpp
        base/metrics.h base/metrics.cpp
)
TARGET_LINK_LIBRARIES(http_test
        GTest::gtest GTest::gtest_main
        Threads::Threads
        glog::glog
        prometheus-cpp::core prometheus-cpp::pull
        ${ABSL_LIBRARIES}
        OpenSSL::Crypto
        cpprestsdk::cpprest
)

ADD_EXECUTABLE(hafnertec_client_test
        hafnertec/hafnertec_client_test.cpp
        base/metrics.h base/metrics.cpp
        ${HTTP_SRC} ${PROTO_SRC})
TARGET_LINK_LIBRARIES(hafnertec_client_test
        hafnertec_client
        GTest::gtest GTest::gtest_main
        Threads::Threads
        gumbo gumbo_query
        glog::glog
        prometheus-cpp::core prometheus-cpp::pull
        ${ABSL_LIBRARIES}
        OpenSSL::Crypto
        ${Protobuf_LIBRARIES}
        cpprestsdk::cpprest
)

ADD_EXECUTABLE(senec_client_test
        senec/senec_client_test.cpp
        base/metrics.h base/metrics.cpp
        ${HTTP_SRC} ${PROTO_SRC})
TARGET_LINK_LIBRARIES(senec_client_test
        senec_client
        GTest::gtest GTest::gtest_main
        Threads::Threads
        gumbo gumbo_query
        glog::glog
        prometheus-cpp::core prometheus-cpp::pull
        ${ABSL_LIBRARIES}
        OpenSSL::Crypto
        ${Protobuf_LIBRARIES}
        cpprestsdk::cpprest
)

gtest_discover_tests(modbus_test)
gtest_discover_tests(http_test)
gtest_discover_tests(hafnertec_client_test)
gtest_discover_tests(senec_client_test)

#ADD_EXECUTABLE(hue_debug hue/hue_debug.cpp)
#TARGET_LINK_LIBRARIES(hue_debug PUBLIC hue_client ${HUE_LIBRARY} ${GLOG_LIBRARY} ${Protobuf_LIBRARIES} config glog::glog crypto absl_strings absl_status absl_hash absl_city absl_raw_hash_set absl_throw_delegate)