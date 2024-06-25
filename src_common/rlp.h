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

/**
 * @brief Decode the length of an RLP encoded field.
 *
 * This function decodes the length of an RLP encoded field, given a buffer containing the RLP encoded data.
 * The RLP encoding format is defined in the Ethereum wiki: https://github.com/ethereum/wiki/wiki/RLP
 *
 * @param [in] buffer The buffer containing the RLP encoded field to decode.
 * @param [in] bufferLength The size of the buffer.
 * @param [out] fieldLength The length of the RLP encoded field.
 * @param [out] offset The offset to the beginning of the RLP encoded field from the buffer.
 * @param [out] list A boolean indicating whether the field encodes a list (true) or a string (false).
 *
 * @return true if the RLP header is consistent, false otherwise.
 */
bool rlpDecodeLength(const uint8_t *buffer, size_t bufferLength, uint32_t *fieldLength, uint32_t *offset, bool *list);