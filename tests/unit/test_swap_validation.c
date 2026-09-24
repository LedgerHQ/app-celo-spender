#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <cmocka.h>

#include "swap_validation.h"

// In swap mode there is no review screen, so every case below is a case where
// the only thing between Exchange's approval and a signature is this function.

#define APPROVED_RECIPIENT "0xabd5D4575341B5878a1E7Cb75Dc0d4DA91DFAFa3"
#define OTHER_RECIPIENT    "0x6cC083aEd9e3ebe302A6336dBC7c921C9f03349E"

// 10000 base units, and a fee ceiling of 100000, both big-endian and minimally
// encoded the way the RLP parser leaves them.
static const uint8_t AMOUNT[] = {0x27, 0x10};
static const uint8_t FEE_CEILING[] = {0x01, 0x86, 0xa0};

// gasprice 100 * startgas 500 = 50000, comfortably inside the ceiling.
static const uint8_t GASPRICE[] = {0x64};
static const uint8_t STARTGAS[] = {0x01, 0xf4};

static const uint8_t TRANSFER_WITH_COMMENT[SELECTOR_LEN] = {0xe1, 0xd6, 0xac, 0xeb};
static const uint8_t VOTE_SELECTOR[SELECTOR_LEN] = {0x58, 0x0d, 0x74, 0x7a};

static txInt256_t int256_of(const uint8_t *bytes, uint8_t length) {
    txInt256_t out;
    memset(&out, 0, sizeof(out));
    memcpy(out.value, bytes, length);
    out.length = length;
    return out;
}

// A native CELO swap as Ledger Live builds it: value transfer, no calldata.
static swap_approved_t approved_native(void) {
    swap_approved_t approved = {
        .ticker = "CELO",
        .decimals = 18,
        .recipient = APPROVED_RECIPIENT,
        .amount = AMOUNT,
        .amount_length = sizeof(AMOUNT),
        .fee = FEE_CEILING,
        .fee_length = sizeof(FEE_CEILING),
    };
    return approved;
}

// Holds the txInt256_t values the swap_tx_t points at.
typedef struct tx_fixture_t {
    swap_tx_t tx;
    txInt256_t value;
    txInt256_t gasprice;
    txInt256_t startgas;
} tx_fixture_t;

static void tx_native(tx_fixture_t *f) {
    memset(f, 0, sizeof(*f));
    f->value = int256_of(AMOUNT, sizeof(AMOUNT));
    f->gasprice = int256_of(GASPRICE, sizeof(GASPRICE));
    f->startgas = int256_of(STARTGAS, sizeof(STARTGAS));
    // The app renders the native ticker with a leading space; Exchange has none.
    f->tx.ticker = " CELO";
    f->tx.decimals = 18;
    f->tx.provisionType = PROVISION_NONE;
    f->tx.selector = NULL;
    f->tx.dataPresent = false;
    f->tx.destination = APPROVED_RECIPIENT;
    f->tx.destinationLength = MAX_ADDRESS;
    f->tx.value = &f->value;
    f->tx.gasprice = &f->gasprice;
    f->tx.startgas = &f->startgas;
    f->tx.feeCurrencyLength = 0;
}

// A token swap: to = token contract, value = 0 on the wire, but finalizeParsing()
// has already replaced destination and value with the ones from the calldata.
static void tx_token(tx_fixture_t *f) {
    tx_native(f);
    f->tx.ticker = "cUSD";
    f->tx.provisionType = PROVISION_TOKEN;
    f->tx.selector = TOKEN_TRANSFER_ID;
    f->tx.dataPresent = false;  // cleared once the transfer is recognised
}

static swap_approved_t approved_token(void) {
    swap_approved_t approved = approved_native();
    approved.ticker = "cUSD";
    return approved;
}

// ---- the two shapes Ledger Live actually produces must be accepted ----

static void test_accepts_native_transfer(void **state) {
    (void) state;
    tx_fixture_t f;
    tx_native(&f);
    const swap_approved_t approved = approved_native();
    assert_true(swap_tx_matches_approval(&f.tx, &approved));
}

static void test_accepts_token_transfer(void **state) {
    (void) state;
    tx_fixture_t f;
    tx_token(&f);
    const swap_approved_t approved = approved_token();
    assert_true(swap_tx_matches_approval(&f.tx, &approved));
}

static void test_accepts_ticker_case_difference(void **state) {
    (void) state;
    // Exchange's configuration and the token descriptor are independent sources;
    // refusing on case alone would break every swap of that asset.
    tx_fixture_t f;
    tx_token(&f);
    f.tx.ticker = "CUSD";
    const swap_approved_t approved = approved_token();
    assert_true(swap_tx_matches_approval(&f.tx, &approved));
}

static void test_accepts_fee_below_ceiling(void **state) {
    (void) state;
    tx_fixture_t f;
    tx_native(&f);
    const uint8_t cheap[] = {0x01};
    f.gasprice = int256_of(cheap, sizeof(cheap));
    const swap_approved_t approved = approved_native();
    assert_true(swap_tx_matches_approval(&f.tx, &approved));
}

// ---- V-003: the asset must be bound ----

static void test_refuses_native_when_token_approved(void **state) {
    (void) state;
    // The approval was for a token, the transaction moves native CELO to the
    // same recipient for the same number of base units.
    tx_fixture_t f;
    tx_native(&f);
    const swap_approved_t approved = approved_token();
    assert_false(swap_tx_matches_approval(&f.tx, &approved));
}

static void test_refuses_token_when_native_approved(void **state) {
    (void) state;
    tx_fixture_t f;
    tx_token(&f);
    const swap_approved_t approved = approved_native();
    assert_false(swap_tx_matches_approval(&f.tx, &approved));
}

static void test_refuses_different_token(void **state) {
    (void) state;
    tx_fixture_t f;
    tx_token(&f);
    f.tx.ticker = "USDT";
    const swap_approved_t approved = approved_token();
    assert_false(swap_tx_matches_approval(&f.tx, &approved));
}

static void test_refuses_decimals_mismatch(void **state) {
    (void) state;
    // Same ticker, different decimals: the amount would mean something else.
    tx_fixture_t f;
    tx_token(&f);
    f.tx.decimals = 6;
    const swap_approved_t approved = approved_token();
    assert_false(swap_tx_matches_approval(&f.tx, &approved));
}

// ---- V-029: only an approvable method ----

static void test_refuses_unrecognised_calldata(void **state) {
    (void) state;
    tx_fixture_t f;
    tx_native(&f);
    f.tx.dataPresent = true;
    const swap_approved_t approved = approved_native();
    assert_false(swap_tx_matches_approval(&f.tx, &approved));
}

static void test_refuses_transfer_with_comment(void **state) {
    (void) state;
    // The comment is signed and, in swap mode, never displayed anywhere.
    tx_fixture_t f;
    tx_token(&f);
    f.tx.selector = TRANSFER_WITH_COMMENT;
    const swap_approved_t approved = approved_token();
    assert_false(swap_tx_matches_approval(&f.tx, &approved));
}

static void test_refuses_staking_calldata(void **state) {
    (void) state;
    // The case that needs no blind signing: finalizeParsing() fills destination
    // and value from the vote calldata, so both comparisons would pass.
    const provision_type_t staking[] = {
        PROVISION_LOCK,     PROVISION_VOTE,   PROVISION_ACTIVATE,
        PROVISION_REVOKE,   PROVISION_UNLOCK, PROVISION_WITHDRAW,
        PROVISION_RELOCK,   PROVISION_CREATE_ACCOUNT,
    };
    const swap_approved_t approved = approved_native();

    for (size_t i = 0; i < sizeof(staking) / sizeof(staking[0]); i++) {
        tx_fixture_t f;
        tx_native(&f);
        f.tx.provisionType = staking[i];
        f.tx.selector = VOTE_SELECTOR;
        f.tx.dataPresent = true;
        assert_false(swap_tx_matches_approval(&f.tx, &approved));
    }
}

// ---- V-008: the fee must be payable in the approved denomination ----

static void test_refuses_token_fee_currency(void **state) {
    (void) state;
    tx_fixture_t f;
    tx_native(&f);
    f.tx.feeCurrencyLength = MAX_ADDRESS;
    const swap_approved_t approved = approved_native();
    assert_false(swap_tx_matches_approval(&f.tx, &approved));
}

// ---- the checks that already existed, kept covered by the extraction ----

static void test_refuses_wrong_recipient(void **state) {
    (void) state;
    tx_fixture_t f;
    tx_native(&f);
    f.tx.destination = OTHER_RECIPIENT;
    const swap_approved_t approved = approved_native();
    assert_false(swap_tx_matches_approval(&f.tx, &approved));
}

static void test_refuses_contract_creation(void **state) {
    (void) state;
    tx_fixture_t f;
    tx_native(&f);
    f.tx.destinationLength = 0;
    const swap_approved_t approved = approved_native();
    assert_false(swap_tx_matches_approval(&f.tx, &approved));
}

static void test_refuses_wrong_amount(void **state) {
    (void) state;
    tx_fixture_t f;
    tx_native(&f);
    const uint8_t more[] = {0x27, 0x11};
    f.value = int256_of(more, sizeof(more));
    const swap_approved_t approved = approved_native();
    assert_false(swap_tx_matches_approval(&f.tx, &approved));
}

static void test_refuses_amount_length_mismatch(void **state) {
    (void) state;
    // Same numeric value, padded: an encoding the parser can produce but the
    // byte comparison must not silently accept.
    tx_fixture_t f;
    tx_native(&f);
    const uint8_t padded[] = {0x00, 0x27, 0x10};
    f.value = int256_of(padded, sizeof(padded));
    const swap_approved_t approved = approved_native();
    assert_false(swap_tx_matches_approval(&f.tx, &approved));
}

static void test_refuses_fee_above_ceiling(void **state) {
    (void) state;
    tx_fixture_t f;
    tx_native(&f);
    const uint8_t expensive[] = {0xff, 0xff};
    f.gasprice = int256_of(expensive, sizeof(expensive));
    const swap_approved_t approved = approved_native();
    assert_false(swap_tx_matches_approval(&f.tx, &approved));
}

static void test_accepts_fee_exactly_at_ceiling(void **state) {
    (void) state;
    // The comparison is gt256, so the ceiling itself is allowed.
    tx_fixture_t f;
    tx_native(&f);
    const uint8_t one[] = {0x01};
    f.gasprice = int256_of(one, sizeof(one));
    f.startgas = int256_of(FEE_CEILING, sizeof(FEE_CEILING));
    const swap_approved_t approved = approved_native();
    assert_true(swap_tx_matches_approval(&f.tx, &approved));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_accepts_native_transfer),
        cmocka_unit_test(test_accepts_token_transfer),
        cmocka_unit_test(test_accepts_ticker_case_difference),
        cmocka_unit_test(test_accepts_fee_below_ceiling),
        cmocka_unit_test(test_refuses_native_when_token_approved),
        cmocka_unit_test(test_refuses_token_when_native_approved),
        cmocka_unit_test(test_refuses_different_token),
        cmocka_unit_test(test_refuses_decimals_mismatch),
        cmocka_unit_test(test_refuses_unrecognised_calldata),
        cmocka_unit_test(test_refuses_transfer_with_comment),
        cmocka_unit_test(test_refuses_staking_calldata),
        cmocka_unit_test(test_refuses_token_fee_currency),
        cmocka_unit_test(test_refuses_wrong_recipient),
        cmocka_unit_test(test_refuses_contract_creation),
        cmocka_unit_test(test_refuses_wrong_amount),
        cmocka_unit_test(test_refuses_amount_length_mismatch),
        cmocka_unit_test(test_refuses_fee_above_ceiling),
        cmocka_unit_test(test_accepts_fee_exactly_at_ceiling),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
