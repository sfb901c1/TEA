# Author: Alexander S.
cmake_minimum_required(VERSION 3.9)
project(login_server)

######################## enclave login_server (trusted part) #############################
add_library(login_server_trusted
        trusted/server_enclave.cpp
        ../common/cryptlib.cpp
        ../common/tee_functions.cpp
        ../common/tee_crypto_functions.cpp)

target_include_directories(login_server_trusted PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/trusted
        ${PROJECT_SOURCE_DIR}/../common
        )

### ZEROMQ and CPPZMQ DEPENDENCIES ###
find_package(cppzmq REQUIRED)
include_directories(${cppzmq_INCLUDE_DIR})

######################## login_server (untrusted part) #############################

### BUILD APP ###
add_executable(login_server
        untrusted/main.cpp
        untrusted/enclave_u_substitute.cpp
        untrusted/network/network_manager_server.cpp
        untrusted/server.cpp)

target_include_directories(login_server PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/untrusted)

target_link_libraries(login_server login_server_trusted ${ZeroMQ_LIBRARY} ${cppzmq_LIBRARY})
add_dependencies(login_server login_server_trusted)
