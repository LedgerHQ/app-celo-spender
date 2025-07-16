#pragma once

#include <stdint.h>

typedef enum {
    SIGN_MODE_BASIC = 0,
    SIGN_MODE_STORE = 1,
    SIGN_MODE_START_FLOW = 2,
} e_sign_mode;

int handleSign(uint8_t p1,
                    uint8_t p2,
                    const uint8_t *workBuffer,
                    uint16_t dataLength,
                    volatile unsigned int *flags);
