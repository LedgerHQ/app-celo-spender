#pragma once

#include <stdint.h>

void handleProvideErc20TokenInformation(uint8_t p1,
                                        uint8_t p2,
                                        uint8_t *workBuffer,
                                        uint16_t dataLength,
                                        volatile unsigned int *flags,
                                        volatile unsigned int *tx);