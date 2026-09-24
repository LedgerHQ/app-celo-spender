cmake_minimum_required(VERSION 3.10)

set (CMAKE_C_STANDARD 11)

project(celo-tests C)

enable_testing()

add_compile_definitions(TESTING)

set(COMMON_SRC "../../src_common")
include_directories(${COMMON_SRC})

add_executable(test_uint256
    test_uint256.c
    ${COMMON_SRC}/uint256.c
    )
target_link_libraries(test_uint256 PRIVATE cmocka)
add_test(NAME test_uint256 COMMAND test_uint256)

add_executable(test_tx_parser
    test_tx_parser.c
    ${COMMON_SRC}/ethUstream.c
    ${COMMON_SRC}/rlp.c
    )
target_link_libraries(test_tx_parser PRIVATE cmocka)
add_test(NAME test_tx_parser COMMAND test_tx_parser)

add_executable(test_eth_utils
    test_eth_utils.c
    ${COMMON_SRC}/ethUtils.c
    )
target_link_libraries(test_eth_utils PRIVATE cmocka)
add_test(NAME test_eth_utils COMMAND test_eth_utils)

# The swap gate is the only check between Exchange's approval and a signature,
# and swap mode cannot be driven from this repo's Ragger tests, so its decision
# table is covered here instead.
set(APP_SRC "../../src")
add_executable(test_swap_validation
    test_swap_validation.c
    ${APP_SRC}/swap/swap_validation.c
    ${COMMON_SRC}/strcase.c
    ${COMMON_SRC}/token_methods.c
    ${COMMON_SRC}/uint256.c
    )
target_include_directories(test_swap_validation PRIVATE ${APP_SRC} ${APP_SRC}/swap)
target_link_libraries(test_swap_validation PRIVATE cmocka)
add_test(NAME test_swap_validation COMMAND test_swap_validation)
