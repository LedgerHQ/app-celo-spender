#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/**
 * @brief Decode an RLP encoded field - see
 * https://github.com/ethereum/wiki/wiki/RLP
 * @param [in] buffer buffer containing the RLP encoded field to decode
 * @param [in] bufferLength size of the buffer
 * @param [out] fieldLength length of the RLP encoded field
 * @param [out] offset offset to the beginning of the RLP encoded field from the
 * buffer
 * @param [out] list true if the field encodes a list, false if it encodes a
 * string
 * @return true if the RLP header is consistent
 */
bool rlpCanDecode(const uint8_t *buffer, size_t bufferLength, bool *valid);
bool rlpDecodeLength(const uint8_t *buffer, size_t bufferLength, uint32_t *fieldLength, uint32_t *offset, bool *list);
