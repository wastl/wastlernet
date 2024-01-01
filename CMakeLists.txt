cmake_minimum_required(VERSION 3.18)
project(wastlernet)

INCLUDE(FindOpenSSL)
INCLUDE(FindThreads)
INCLUDE(GoogleTest)
INCLUDE(FindPostgreSQL)

set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_library(MODBUS_LIBRARY NAMES libmodbus modbus)
find_library(PQXX_LIBRARY NAMES libpqxx pqxx)

ADD_SUBDIRECTORY(config)
ADD_SUBDIRECTORY(hafnertec)
ADD_SUBDIRECTORY(senec)
ADD_SUBDIRECTORY(solvis)
ADD_SUBDIRECTORY(weather)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

ADD_EXECUTABLE(wastlernet main.cpp include/module.h include/updater.h include/utility.h include/utility.cpp)
TARGET_LINK_LIBRARIES(wastlernet
        config
        hafnertec_client solvis_client senec_client weather_client
        cpprest pthread glog gumbo gumbo_query
        absl_strings absl_status absl_statusor absl_str_format_internal absl_throw_delegate absl_hash absl_city
        absl_raw_hash_set absl_synchronization
        crypto pqxx
        ${PostgreSQL_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${OPENSSL_SSL_LIBRARIES}
        ${Protobuf_LIBRARIES}
        ${MODBUS_LIBRARY})