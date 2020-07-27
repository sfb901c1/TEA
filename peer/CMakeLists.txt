# Author: Alexander S.
cmake_minimum_required(VERSION 3.9)
project(peer)

### JSONCPP DEPENDENCIES ###
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)

######################## peer_trusted lib (trusted part) #############################
add_library(peer_trusted
        ../common/tee_crypto_functions.cpp
        ../common/tee_functions.cpp
        trusted/peer_enclave.cpp
        trusted/overlay_structure_scheme.cpp
        trusted/distributed_agreement_scheme.cpp
        trusted/routing_scheme.cpp
        ../common/cryptlib.cpp
        shared/overlay_structure_scheme_message.cpp
        shared/overlay_return_tuple.cpp)

target_include_directories(peer_trusted PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/trusted
        ${JSONCPP_INCLUDE_DIRS})

target_compile_definitions(peer_trusted PRIVATE BUILD_WITH_VISUALIZATION)

######################## peer_untrusted lib (untrusted part) #############################

### ZEROMQ and CPPZMQ DEPENDENCIES ###
find_package(cppzmq REQUIRED)

### CURLPP DEPENDENCIES ###
find_package(CURL)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(cURLpp REQUIRED)

add_library(peer_untrusted
        untrusted/main.cpp
        untrusted/enclave_u_substitute.cpp
        untrusted/network/network_manager.cpp
        untrusted/peer.cpp
        shared/overlay_structure_scheme_message.cpp
        shared/overlay_return_tuple.cpp)

option(BUILD_WITH_VISUALIZATION "Build with visualization" OFF)
if(BUILD_WITH_VISUALIZATION)
target_compile_definitions(peer_untrusted PRIVATE BUILD_WITH_VISUALIZATION)
endif()

target_include_directories(peer_untrusted PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/untrusted
        ${CURL_INCLUDE_DIRS}
        ${CURLPP_INCLUDE_DIR}
        ${JSONCPP_INCLUDE_DIRS}
        ${cppzmq_INCLUDE_DIR})

target_link_libraries(peer_untrusted ${CURL_LIBRARIES}
        ${CURLPP_LIBRARY}
        ${ZeroMQ_LIBRARY}
        ${cppzmq_LIBRARY})

######################## peer (app) #############################  

add_executable(peer untrusted/main.cpp)

target_link_libraries(peer
        peer_untrusted
        peer_trusted
        ${JSONCPP_LIBRARIES})

add_dependencies(peer peer_trusted peer_untrusted)
