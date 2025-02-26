#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <stdbool.h>

#include "../src_common/ethUstream.h"

static void test_process_tx(uint8_t txType, const uint8_t* tx_data, size_t tx_data_size, const uint8_t* expected_to, int expected_status) {
  txContext_t context;
  txContent_t content;
  cx_sha3_t sha3;

  initTx(&context, &sha3, &content, NULL, NULL);
  context.txType = txType;
  assert_int_equal(processTx(&context, tx_data, tx_data_size), expected_status);
  // do not perform other checks if expected_to is NULL
  if(expected_to == NULL) {
    return;
  }
  assert_int_equal(content.destinationLength, MAX_ADDRESS);
  assert_memory_equal(content.destination, expected_to, MAX_ADDRESS);
}


static void test_process_tx_eip1559(void **state) {
  (void) state;
  const uint8_t tx_data[] = {
    0xed, 0x82, 0xa4, 0xec, 0x80, 0x84, 0x77, 0x35, 0x94, 0x00, 0x85, 0x02,
    0xcb, 0x41, 0x78, 0x00, 0x82, 0x6a, 0xa4, 0x94, 0xda, 0x52, 0xc9, 0xff,
    0xeb, 0xd4, 0xd5, 0x4c, 0x94, 0xa0, 0x72, 0x77, 0x61, 0x26, 0x06, 0x9d,
    0x43, 0xe7, 0x4f, 0x9e, 0x80, 0x80, 0xc0, 0x01, 0x80, 0x80
  };

  const uint8_t to[] = {
    0xda, 0x52, 0xc9, 0xff, 0xeb, 0xd4, 0xd5, 0x4c, 0x94, 0xa0, 0x72, 0x77,
    0x61, 0x26, 0x06, 0x9d, 0x43, 0xe7, 0x4f, 0x9e
  };

  test_process_tx(EIP1559, tx_data, sizeof(tx_data), to, USTREAM_FINISHED);
}

static void test_process_tx_cip64(void **state) {
  (void) state;
  const uint8_t tx_data[] = {
    0xf8, 0x43, 0x82, 0xa4, 0xec, 0x80, 0x84, 0x77, 0x35, 0x94, 0x00, 0x85,
    0x03, 0x86, 0xc6, 0x34, 0xf7, 0x83, 0x01, 0x68, 0x8c, 0x94, 0xda, 0x52,
    0xc9, 0xff, 0xeb, 0xd4, 0xd5, 0x4c, 0x94, 0xa0, 0x72, 0x77, 0x61, 0x26,
    0x06, 0x9d, 0x43, 0xe7, 0x4f, 0x9e, 0x80, 0x80, 0xc0, 0x94, 0x76, 0x5d,
    0xe8, 0x16, 0x84, 0x58, 0x61, 0xe7, 0x5a, 0x25, 0xfc, 0xa1, 0x22, 0xbb,
    0x68, 0x98, 0xb8, 0xb1, 0x28, 0x2a, 0x01, 0x80, 0x80
  };

  const uint8_t to[] = {
    0xda, 0x52, 0xc9, 0xff, 0xeb, 0xd4, 0xd5, 0x4c, 0x94, 0xa0, 0x72, 0x77,
    0x61, 0x26, 0x06, 0x9d, 0x43, 0xe7, 0x4f, 0x9e
  };

  test_process_tx(CIP64, tx_data, sizeof(tx_data), to, USTREAM_FINISHED);
}


int main(void) {
    const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_process_tx_eip1559),
      cmocka_unit_test(test_process_tx_cip64)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
