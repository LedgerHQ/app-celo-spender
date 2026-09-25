#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <cmocka.h>

#include "ethUtils.h"

// Size of the buffer finalizeParsing() formats amounts into, in src/celo.c. The bound
// matters: it is what decides whether adjustDecimals() can fail on a given input.
#define CELO_AMOUNT_TARGET_LEN 100

// With srcLength == 1, adjustDecimals() needs 1 + 1 + 2 + (decimals - 1) bytes, so a
// 100-byte target holds up to 97 decimals and no more.
#define MAX_DECIMALS_THAT_FIT 97

static void test_adjust_decimals_common_case(void **state) {
    (void) state;
    char target[CELO_AMOUNT_TARGET_LEN];

    assert_true(adjustDecimals("10000", 5, target, sizeof(target), 18));
    assert_string_equal(target, "0.00000000000001");
}

static void test_adjust_decimals_refuses_when_target_too_small(void **state) {
    (void) state;
    char target[CELO_AMOUNT_TARGET_LEN];

    // A token whose decimals exceed what the amount buffer can hold. The value is not
    // representable, so adjustDecimals() reports failure instead of writing a partial
    // amount -- which is exactly why its callers must check the return value.
    assert_false(adjustDecimals("1", 1, target, sizeof(target), UINT8_MAX));
}

static void test_adjust_decimals_target_boundary(void **state) {
    (void) state;
    char target[CELO_AMOUNT_TARGET_LEN];

    assert_true(adjustDecimals("1", 1, target, sizeof(target), MAX_DECIMALS_THAT_FIT));
    assert_false(adjustDecimals("1", 1, target, sizeof(target), MAX_DECIMALS_THAT_FIT + 1));
}

static void test_adjust_decimals_leaves_target_unusable_on_failure(void **state) {
    (void) state;
    char target[CELO_AMOUNT_TARGET_LEN];

    // A caller that ignores the return value prints whatever was already in the buffer.
    // This pins that behaviour so the guards at the call sites are not removed as
    // redundant.
    char untouched[CELO_AMOUNT_TARGET_LEN];
    memset(untouched, 'X', sizeof(untouched) - 1);
    untouched[sizeof(untouched) - 1] = '\0';
    memcpy(target, untouched, sizeof(target));

    assert_false(adjustDecimals("1", 1, target, sizeof(target), UINT8_MAX));
    assert_string_equal(target, untouched);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_adjust_decimals_common_case),
        cmocka_unit_test(test_adjust_decimals_refuses_when_target_too_small),
        cmocka_unit_test(test_adjust_decimals_target_boundary),
        cmocka_unit_test(test_adjust_decimals_leaves_target_unusable_on_failure),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
