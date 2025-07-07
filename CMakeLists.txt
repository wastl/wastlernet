cmake_minimum_required(VERSION 3.27)
project(wastlernet)

INCLUDE(FindOpenSSL)
INCLUDE(FindThreads)
INCLUDE(GoogleTest)
INCLUDE(FindPostgreSQL)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package (glog 0.7.1 REQUIRED)
add_compile_definitions(-DGLOG_USE_GLOG_EXPORT)

find_package(Protobuf REQUIRED)
find_library(MODBUS_LIBRARY NAMES libmodbus modbus)
find_library(PQXX_LIBRARY NAMES libpqxx pqxx)

find_package(hueplusplus REQUIRED)
find_library(HUE_LIBRARY NAMES hueplusplusshared hueplusplusstatic)

enable_testing()
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

SET(ABSL_LIBRARIES absl_strings absl_status absl_statusor absl_str_format_internal absl_throw_delegate absl_hash absl_city
        absl_raw_hash_set absl_synchronization)

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
        cpprest pthread glog::glog gumbo gumbo_query
        absl_strings absl_status absl_statusor absl_str_format_internal absl_throw_delegate absl_hash absl_city
        absl_raw_hash_set absl_synchronization
        crypto pqxx
        ${PostgreSQL_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${OPENSSL_SSL_LIBRARIES}
        ${Protobuf_LIBRARIES}
        ${MODBUS_LIBRARY})

ADD_EXECUTABLE(modbus_test
        include/modbus_connection_test.cpp
        include/modbus_connection.h include/modbus_connection.cpp
        include/utility.h include/utility.cpp
)
TARGET_LINK_LIBRARIES(modbus_test
        GTest::gtest GTest::gtest_main
        pthread glog::glog
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
        pthread glog::glog
        ${ABSL_LIBRARIES}
        cpprest crypto
)

gtest_discover_tests(modbus_test)
gtest_discover_tests(http_test)

#ADD_EXECUTABLE(hue_debug hue/hue_debug.cpp)
#TARGET_LINK_LIBRARIES(hue_debug PUBLIC hue_client ${HUE_LIBRARY} ${GLOG_LIBRARY} ${Protobuf_LIBRARIES} config glog::glog crypto absl_strings absl_status absl_hash absl_city absl_raw_hash_set absl_throw_delegate)