/*******************************************************************************
*   Ledger Ethereum App
*   (c) 2016-2019 Ledger
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/
#pragma once

#include <stdint.h>

#include "uint256.h"

/**
 * Converts a binary array to a hexadecimal string representation.
 *
 * @param strbuf [out] The buffer to store the hexadecimal string.
 * @param bin [in] The binary array to convert.
 * @param len [in] The length of the binary array.
 */
void array_hexstr(char *strbuf, const void *bin, size_t len);

/**
 * Converts a big-endian byte array to a uint256_t target.
 *
 * @param data [in] The big-endian byte array to convert.
 * @param length [in] The length of the byte array.
 * @param target [out] The target uint256_t to store the converted value.
 */
void convertUint256BE(const uint8_t *data, uint32_t length, uint256_t *target);

/**
 * Retrieves the V value from the transaction content.
 *
 * @param txContent [in] The transaction content.
 * @return The V value.
 */
uint32_t getV(txContent_t *txContent);


/**
 * Parse BIP32 path
 *
 * @param derivationPath The BIP32 path structure to be filled
 * @param input The input data buffer
 * @param len The length of the input data buffer
 * @return 0 if successful, -1 if failed
 */
int parse_bip32_path(bip32Path_t *derivationPath, const uint8_t *input, size_t len);