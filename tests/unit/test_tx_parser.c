#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <stdbool.h>
#include <string.h>

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



// ---- chain ID authorization ----
//
// The chain ID is the only thing tying a typed-transaction signature to a
// network, and the app never shows it, so these cases are the whole of that
// restriction.

// Everything in an EIP-1559 Celo transaction after the chain ID: nonce,
// maxPriorityFeePerGas, maxFeePerGas, gasLimit, to, value, data, accessList and
// the trailing placeholders. Taken from the known-good vector above.
static const uint8_t EIP1559_TAIL[] = {
  0x80, 0x84, 0x77, 0x35, 0x94, 0x00, 0x85, 0x02, 0xcb, 0x41, 0x78, 0x00,
  0x82, 0x6a, 0xa4, 0x94, 0xda, 0x52, 0xc9, 0xff, 0xeb, 0xd4, 0xd5, 0x4c,
  0x94, 0xa0, 0x72, 0x77, 0x61, 0x26, 0x06, 0x9d, 0x43, 0xe7, 0x4f, 0x9e,
  0x80, 0x80, 0xc0, 0x01, 0x80, 0x80
};

// Assembles the transaction RLP around a caller-supplied, already-encoded chain
// ID field, so a test can vary that one field without hand-editing the list
// header that depends on its length.
static size_t build_eip1559(uint8_t* out, size_t outSize,
                            const uint8_t* chainIdField, size_t chainIdFieldLen) {
  const size_t payloadLen = chainIdFieldLen + sizeof(EIP1559_TAIL);
  // A longer payload would need a multi-byte list header, which none of these
  // vectors require; assert rather than emit a malformed one.
  assert_true(payloadLen < 56);
  assert_true(outSize >= payloadLen + 1);

  out[0] = 0xc0 + payloadLen;
  memcpy(out + 1, chainIdField, chainIdFieldLen);
  memcpy(out + 1 + chainIdFieldLen, EIP1559_TAIL, sizeof(EIP1559_TAIL));
  return payloadLen + 1;
}

static void assert_chain_id(const uint8_t* chainIdField, size_t chainIdFieldLen,
                            int expected_status) {
  uint8_t tx[64];
  const size_t txLen = build_eip1559(tx, sizeof(tx), chainIdField, chainIdFieldLen);
  test_process_tx(EIP1559, tx, txLen, NULL, expected_status);
}

static void test_chain_id_authorized_networks(void **state) {
  (void) state;
  const uint8_t mainnet[]   = {0x82, 0xa4, 0xec};        // 42220
  const uint8_t alfajores[] = {0x82, 0xae, 0xf3};        // 44787
  const uint8_t baklava[]   = {0x82, 0xf3, 0x70};        // 62320
  // The only authorized chain ID that does not fit in two bytes, so it also
  // covers the widths the comparison has to handle.
  const uint8_t celo_sepolia[] = {0x83, 0xaa, 0x04, 0x4c};  // 11142220

  assert_chain_id(mainnet, sizeof(mainnet), USTREAM_FINISHED);
  assert_chain_id(alfajores, sizeof(alfajores), USTREAM_FINISHED);
  assert_chain_id(baklava, sizeof(baklava), USTREAM_FINISHED);
  assert_chain_id(celo_sepolia, sizeof(celo_sepolia), USTREAM_FINISHED);
}

static void test_chain_id_unassigned_value_is_refused(void **state) {
  (void) state;
  // 17323 (0x43AB) sat in the allowlist as "Baklava" but is not Baklava and is
  // not a registered chain at all; Baklava is 62320.
  const uint8_t unassigned[] = {0x82, 0x43, 0xab};
  assert_chain_id(unassigned, sizeof(unassigned), USTREAM_FAULT);
}

static void test_chain_id_longer_than_authorized_is_refused(void **state) {
  (void) state;
  // The reported bug: only the first two bytes were compared, so any chain ID
  // starting with an authorized one was accepted.
  const uint8_t three_bytes[] = {0x83, 0xa4, 0xec, 0x01};        // 0xA4EC01
  const uint8_t four_bytes[]  = {0x84, 0xae, 0xf3, 0xff, 0xff};  // 0xAEF3FFFF

  assert_chain_id(three_bytes, sizeof(three_bytes), USTREAM_FAULT);
  assert_chain_id(four_bytes, sizeof(four_bytes), USTREAM_FAULT);
}

static void test_chain_id_empty_is_refused(void **state) {
  (void) state;
  // RLP encodes zero as an empty string, which left the field length at 0 and
  // skipped the authorization check altogether.
  const uint8_t empty[] = {0x80};
  assert_chain_id(empty, sizeof(empty), USTREAM_FAULT);
}

static void test_chain_id_non_canonical_is_refused(void **state) {
  (void) state;
  // 0x00A4EC decodes to an authorized value but is not how RLP encodes it, so
  // accepting it would mean two encodings authorize the same signature.
  const uint8_t leading_zero[] = {0x83, 0x00, 0xa4, 0xec};
  assert_chain_id(leading_zero, sizeof(leading_zero), USTREAM_FAULT);
}

static void test_chain_id_unauthorized_is_refused(void **state) {
  (void) state;
  const uint8_t ethereum_mainnet[] = {0x01};
  const uint8_t unrelated[]        = {0x82, 0x13, 0x88};  // 5000

  assert_chain_id(ethereum_mainnet, sizeof(ethereum_mainnet), USTREAM_FAULT);
  assert_chain_id(unrelated, sizeof(unrelated), USTREAM_FAULT);
}

static void test_chain_id_split_across_apdus_is_accepted(void **state) {
  (void) state;
  // The check used to run on each chunk, so an authorized chain ID arriving in
  // two pieces was judged on the first byte alone and refused.
  const uint8_t mainnet[] = {0x82, 0xa4, 0xec};
  uint8_t tx[64];
  const size_t txLen = build_eip1559(tx, sizeof(tx), mainnet, sizeof(mainnet));

  txContext_t context;
  txContent_t content;
  cx_sha3_t sha3;
  initTx(&context, &sha3, &content, NULL, NULL);
  context.txType = EIP1559;

  // Splits after the list header and the chain ID's own header plus one byte,
  // leaving the field incomplete.
  const size_t split = 3;
  assert_int_equal(processTx(&context, tx, split), USTREAM_PROCESSING);
  assert_int_equal(processTx(&context, tx + split, txLen - split), USTREAM_FINISHED);
}

int main(void) {
    const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_process_tx_eip1559),
      cmocka_unit_test(test_process_tx_cip64),
      cmocka_unit_test(test_chain_id_authorized_networks),
      cmocka_unit_test(test_chain_id_unassigned_value_is_refused),
      cmocka_unit_test(test_chain_id_longer_than_authorized_is_refused),
      cmocka_unit_test(test_chain_id_empty_is_refused),
      cmocka_unit_test(test_chain_id_non_canonical_is_refused),
      cmocka_unit_test(test_chain_id_unauthorized_is_refused),
      cmocka_unit_test(test_chain_id_split_across_apdus_is_accepted)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
