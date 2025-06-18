#pragma once

#include <stdint.h>

int handleSignPersonalMessage(uint8_t p1,
                              uint8_t p2,
                              uint8_t *workBuffer,
                              uint16_t dataLength,
                              volatile unsigned int *flags,
                              volatile unsigned int *tx);
