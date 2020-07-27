# Author: Alexander S.

cmake_minimum_required(VERSION 3.9)
project(c1)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(peer)
add_subdirectory(login_server)
#add_subdirectory(client_interface)
#add_subdirectory(qt_client_interface)
#add_subdirectory(qt_clients_interface)

enable_testing()
add_subdirectory(tests)
#add_test(shared_structs_test shared_structs_test)
#add_test(peer_test peer_test)