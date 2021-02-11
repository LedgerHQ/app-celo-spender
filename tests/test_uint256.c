#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

#include "uint256.h"

#define assert_uint256_equal(a, b) assert_memory_equal((a), (b), sizeof(uint256_t))

static void uint256_from_uint(uint256_t *n, uint64_t v) {
  n->elements[0].elements[0] = 0;
  n->elements[0].elements[1] = 0;
  n->elements[1].elements[0] = 0;
  n->elements[1].elements[1] = v;
}

static void test_bitshift_right(void **state) {
  (void) state;
  const uint256_t val = {0, 0, 0, 0xffffffffffffffffULL};
  uint256_t r;
  for (int i = 0; i < 64; i++) {
    uint256_t expected = {0, 0, 0, 0xffffffffffffffffULL >> i};
    shiftr256(&val, i, &r);
    assert_uint256_equal(&r, &expected);
  }

  const uint256_t zero = {0};
  for (int i = 0; i < 64; i++) {
    shiftr256(&zero, i, &r);
    assert_uint256_equal(&r, &zero);
  }
}

static void test_bitshift_left(void **state) {
  (void) state;

  uint256_t val;
  uint256_from_uint(&val, 1);

  uint256_t r, expected;
  for (int i = 0; i < 64; i++) {
    shiftl256(&val, i, &r);
    uint256_from_uint(&expected, 1ULL << i);
    assert_uint256_equal(&r, &expected);
  }

  const uint256_t zero = {0};
  for (int i = 0; i < 64; i++) {
    shiftl256(&zero, i, &r);
    assert_uint256_equal(&r, &zero);
  }
}

static void test_external_shift_right(void **state) {
  (void) state;
  uint256_t t = {0, 0, 0, 1};
  uint256_t f = {0};
  uint256_t u8, u16, u32, u64;

  uint256_from_uint(&u8, 0xffULL);
  uint256_from_uint(&u16, 0xffffULL);
  uint256_from_uint(&u32, 0xffffffffULL);
  uint256_from_uint(&u64, 0xffffffffffffffffULL);

  const uint256_t zero = {0};
  const uint256_t one = {0, 0, 0, 1};

  uint256_t r;
  shiftr256(&t, 0, &r);
  assert_uint256_equal(&r, &one);
  shiftr256(&f, 0, &r);
  assert_uint256_equal(&r, &zero);
  shiftr256(&u8, 0, &r);
  assert_uint256_equal(&r, &u8);
  shiftr256(&u16, 0, &r);
  assert_uint256_equal(&r, &u16);
  shiftr256(&u32, 0, &r);
  assert_uint256_equal(&r, &u32);
  shiftr256(&u64, 0, &r);
  assert_uint256_equal(&r, &u64);

  shiftr256(&t, 1, &t);
  assert_uint256_equal(&t, &zero);
  shiftr256(&f, 1, &f);
  assert_uint256_equal(&f, &zero);

  shiftr256(&u8, 1, &u8);
  uint256_from_uint(&r, 0x7fULL);
  assert_uint256_equal(&u8, &r);
  shiftr256(&u16, 1, &u16);
  uint256_from_uint(&r, 0x7fffULL);
  assert_uint256_equal(&u16, &r);
  shiftr256(&u32, 1, &u32);
  uint256_from_uint(&r, 0x7fffffffULL);
  assert_uint256_equal(&u32, &r);
  shiftr256(&u64, 1, &u64);
  uint256_from_uint(&r, 0x7fffffffffffffffULL);
  assert_uint256_equal(&u64, &r);

  shiftr256(&u8, 7, &u8);
  assert_uint256_equal(&u8, &zero);
  shiftr256(&u16, 15, &u16);
  assert_uint256_equal(&u16, &zero);
  shiftr256(&u32, 31, &u32);
  assert_uint256_equal(&u32, &zero);
  shiftr256(&u64, 63, &u64);
  assert_uint256_equal(&u64, &zero);
}

static void test_external_shift_left(void **state) {
  (void) state;

  uint256_t t = {0, 0, 0, 1};
  uint256_t f = {0};
  uint256_t u8, u16, u32, u64;

  uint256_from_uint(&u8, 0x7fULL);
  uint256_from_uint(&u16, 0x7fffULL);
  uint256_from_uint(&u32, 0x7fffffffULL);
  uint256_from_uint(&u64, 0x7fffffffffffffffULL);
  uint256_t u128 = {0, 0, 0x7fffffffffffffffULL, 0xffffffffffffffffULL};

  const uint256_t zero = {0};
  const uint256_t one = {0, 0, 0, 1};

  uint256_t r;
  shiftl256(&t, 0, &r);
  assert_uint256_equal(&r, &one);
  shiftl256(&f, 0, &r);
  assert_uint256_equal(&r, &zero);  
  shiftl256(&u8, 0, &r);
  assert_uint256_equal(&r, &u8);
  shiftl256(&u16, 0, &r);
  assert_uint256_equal(&r, &u16);
  shiftl256(&u32, 0, &r);
  assert_uint256_equal(&r, &u32);
  shiftl256(&u64, 0, &r);
  assert_uint256_equal(&r, &u64);

  shiftl256(&u8, 1, &u8);
  uint256_from_uint(&r, 0xfeULL);
  assert_uint256_equal(&u8, &r);
  shiftl256(&u16, 1, &u16);
  uint256_from_uint(&r, 0xfffeULL);
  assert_uint256_equal(&u16, &r);
  shiftl256(&u32, 1, &u32);
  uint256_from_uint(&r, 0xfffffffeULL);
  assert_uint256_equal(&u32, &r);
  shiftl256(&u64, 1, &u64);
  uint256_from_uint(&r, 0xfffffffffffffffeULL);
  assert_uint256_equal(&u64, &r);
  uint256_t expected_r128 = {0, 0, 0xffffffffffffffffULL, 0xfffffffffffffffeULL};
  shiftl256(&u128, 1, &u128);
  assert_uint256_equal(&u128, &expected_r128);

  uint256_t r1;
  shiftl256(&u8, 7, &r1);
  uint256_from_uint(&r, 0x7f00ULL);
  assert_uint256_equal(&r, &r1);

  shiftl256(&u16, 15, &r1);
  uint256_from_uint(&r, 0x7fff0000ULL);
  assert_uint256_equal(&r, &r1);

  shiftl256(&u32, 31, &r1);
  uint256_from_uint(&r, 0x7fffffff00000000ULL);
  assert_uint256_equal(&r, &r1);

  shiftl256(&u64, 63, &r1);
  uint256_t r2 = {0, 0, 0x7fffffffffffffffULL, 0};
  assert_uint256_equal(&r1, &r2);

  shiftl256(&u128, 127, &r1);
  uint256_t r3 = {0x7fffffffffffffffULL, 0xffffffffffffffffULL, 0x0000000000000000ULL, 0x0000000000000000ULL};
  assert_uint256_equal(&r1, &r3);
}

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
      cmocka_unit_test(test_bitshift_right),
      cmocka_unit_test(test_bitshift_left),
      cmocka_unit_test(test_external_shift_right),
      cmocka_unit_test(test_external_shift_left),
      cmocka_unit_test(test_arithmetic_multiply),
      cmocka_unit_test(test_external_multiply),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
