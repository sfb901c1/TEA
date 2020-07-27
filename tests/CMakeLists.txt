# Author: Alexander S.
cmake_minimum_required(VERSION 3.9)

find_package(Boost REQUIRED COMPONENTS unit_test_framework)

# shared_structs test
add_executable(shared_structs_test shared_structs_test.cpp)
target_include_directories(shared_structs_test PRIVATE ${BOOST_INCLUDE_DIR})
target_link_libraries(shared_structs_test ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

# peer test
add_executable(peer_test peer_test.cpp ../peer/shared/overlay_structure_scheme_message.cpp)
target_include_directories(peer_test PRIVATE ${BOOST_INCLUDE_DIR})
target_link_libraries(peer_test ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_compile_definitions(peer_test PRIVATE TESTING)

add_test(shared_structs_test shared_structs_test)
add_test(peer_test peer_test)