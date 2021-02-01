cmake_minimum_required(VERSION 3.10)

set (CMAKE_C_STANDARD 11)

project(celo-tests C)

enable_testing()

set(COMMON_SRC "../src_common")
include_directories(${COMMON_SRC})

add_executable(test_uint256 test_uint256.c ${COMMON_SRC}/uint256.c)
target_link_libraries(test_uint256 PRIVATE cmocka)
add_test(NAME test_uint256 COMMAND test_uint256)
