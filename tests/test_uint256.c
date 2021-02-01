#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

#include "uint256.h"

#define assert_uint256_equal(a, b) assert_memory_equal((a), (b), sizeof(uint256_t))

static void test_arithmetic_multiply(void **state) {
  (void) state;

  const uint256_t val = {0, 0, 0, 0xfedbca9876543210ULL};
  const uint256_t zero = {0};
  const uint256_t one = {0, 0, 0, 1};

  uint256_t r1, r2, r3, r4, r5;

  const uint256_t expected_r1 = {0x0000000000000000ULL, 0x0000000000000000ULL, 0xfdb8e2bacbfe7cefULL, 0x010e6cd7a44a4100ULL};
  mul256(&val, &val, &r1);
  assert_uint256_equal(&r1, &expected_r1);

  mul256(&val, &zero, &r2);
  assert_uint256_equal(&r2, &zero);
  mul256(&zero, &val, &r3);
  assert_uint256_equal(&r3, &zero);

  mul256(&val, &one, &r4);
  assert_uint256_equal(&r4, &val);
  mul256(&one, &val, &r5);
  assert_uint256_equal(&r5, &val);
}

static void test_external_multiply(void **state) {
  (void) state;

  const uint256_t f = {0};
  const uint256_t t = {0, 0, 0, 1};
  const uint256_t u8 = {0, 0, 0, 0xaa};
  const uint256_t u16 = {0, 0, 0, 0xaaaa};
  const uint256_t u32 = {0, 0, 0, 0xaaaaaaaa};
  const uint256_t u64 = {0, 0, 0, 0xaaaaaaaaaaaaaaaaULL};
  const uint256_t u128 = {0, 0, 0xaaaaaaaaaaaaaaaaULL, 0xaaaaaaaaaaaaaaaaULL};
  const uint256_t val  = {0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f0f0f0f0f0ULL};

  uint256_t r1, r2, r3, r4, r5, r6, r7;

  const uint256_t expected_r1 = {0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f0f0f0f0f0ULL};
  mul256(&t, &val, &r1);
  assert_uint256_equal(&r1, &expected_r1);

  const uint256_t expected_r2 = {0};
  mul256(&f, &val, &r2);
  assert_uint256_equal(&r2, &expected_r2);

  const uint256_t expected_r3 = {0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffff60ULL};
  mul256(&u8, &val, &r3);
  assert_uint256_equal(&r3, &expected_r3);

  const uint256_t expected_r4 = {0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffff5f60ULL};
  mul256(&u16, &val, &r4);
  assert_uint256_equal(&r4, &expected_r4);

  const uint256_t expected_r5 = {0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffff5f5f5f60ULL};
  mul256(&u32, &val, &r5);
  assert_uint256_equal(&r5, &expected_r5);

  const uint256_t expected_r6 = {0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x5f5f5f5f5f5f5f60ULL};
  mul256(&u64, &val, &r6);
  assert_uint256_equal(&r6, &expected_r6);

  const uint256_t expected_r7 = {0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x5f5f5f5f5f5f5f5fULL, 0x5f5f5f5f5f5f5f60ULL};
  mul256(&u128, &val, &r7);
  assert_uint256_equal(&r7, &expected_r7);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_arithmetic_multiply),
        cmocka_unit_test(test_external_multiply),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
