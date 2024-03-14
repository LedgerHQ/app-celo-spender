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

/**
 * @brief Utilities for an Ethereum Hardware Wallet logic
 * @file ethUtils.h
 * @author Ledger Firmware Team <hello@ledger.fr>
 * @version 1.0
 * @date 8th of March 2016
 */

#include "os.h"
#include "cx.h"
#include "ethUtils.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef CHECKSUM_1

static const uint8_t const HEXDIGITS[] = "0123456789ABCDEF";

static const uint8_t const MASK[] = {0x80, 0x40, 0x20, 0x10,
                                     0x08, 0x04, 0x02, 0x01};

char convertDigit(uint8_t *address, uint8_t index, uint8_t *hash) {
    unsigned char digit = address[index / 2];
    if ((index % 2) == 0) {
        digit = (digit >> 4) & 0x0f;
    } else {
        digit = digit & 0x0f;
    }
    if (digit < 10) {
        return HEXDIGITS[digit];
    } else {
        unsigned char data = hash[index / 8];
        if (((data & MASK[index % 8]) != 0) && (digit > 9)) {
            return HEXDIGITS[digit] /*- 'a' + 'A'*/;
        } else {
            return HEXDIGITS[digit];
        }
    }
}

void getEthAddressStringFromKey(const cx_ecfp_public_key_t *publicKey, char *out,
                                cx_sha3_t *sha3Context) {
    uint8_t hashAddress[32];
    cx_keccak_init_no_throw(sha3Context, 256);
    cx_hash((cx_hash_t*)sha3Context, CX_LAST, publicKey->W + 1, 64, hashAddress, 32);
    getEthAddressStringFromBinary(hashAddress + 12, out, sha3Context);
}

void getEthAddressStringFromBinary(const uint8_t *address, uint8_t *out,
                                   cx_sha3_t *sha3Context) {
    uint8_t hashChecksum[32];
    uint8_t i;
    cx_keccak_init_no_throw(sha3Context, 256);
    cx_hash((cx_hash_t*)sha3Context, CX_LAST, address, 20, hashChecksum, 32);
    for (i = 0; i < 40; i++) {
        out[i] = convertDigit(address, i, hashChecksum);
    }
    out[40] = '\0';
}

#else

static const uint8_t HEXDIGITS[] = "0123456789abcdef";

void getEthAddressStringFromKey(const cx_ecfp_public_key_t *publicKey, char *out, int chainId, cx_sha3_t *sha3Context) {
    uint8_t hashAddress[32];
    cx_keccak_init_no_throw(sha3Context, 256);
    cx_hash((cx_hash_t*)sha3Context, CX_LAST, publicKey->W + 1, 64, hashAddress, 32);
    getEthAddressStringFromBinary(hashAddress + 12, out, chainId, sha3Context);
}

void getEthAddressStringFromBinary(const uint8_t *address, char *out, int chainId, cx_sha3_t *sha3Context) {
    uint8_t hashChecksum[32];
    char tmp[100];
    uint8_t i;
    bool eip1191 = false;
    uint32_t offset = 0;

    switch(chainId) {
        case 30:
        case 31:
            eip1191 = true;
            break;
    }
    if (eip1191) {
        snprintf(tmp, sizeof(tmp), "%d0x", chainId);
        offset = strlen(tmp);
    }
    for (i = 0; i < 20; i++) {
        uint8_t digit = address[i];
        tmp[offset + 2 * i] = HEXDIGITS[(digit >> 4) & 0x0f];
        tmp[offset + 2 * i + 1] = HEXDIGITS[digit & 0x0f];
    }
    cx_keccak_init_no_throw(sha3Context, 256);
    cx_hash((cx_hash_t*)sha3Context, CX_LAST, (uint8_t *) tmp, offset + 40, hashChecksum, 32);
    for (i = 0; i < 40; i++) {
        uint8_t digit = address[i / 2];
        if ((i % 2) == 0) {
            digit = (digit >> 4) & 0x0f;
        } else {
            digit = digit & 0x0f;
        }
        if (digit < 10) {
            out[i] = HEXDIGITS[digit];
        }
        else {
            int v = (hashChecksum[i / 2] >> (4 * (1 - i % 2))) & 0x0f;
            if (v >= 8) {
                out[i] = HEXDIGITS[digit] - 'a' + 'A';
            }
            else {
                out[i] = HEXDIGITS[digit];
            }
        }
    }
    out[40] = '\0';
}

#endif

bool adjustDecimals(const char *src, size_t srcLength, char *target,
                    size_t targetLength, uint8_t decimals) {
    uint32_t startOffset;
    uint32_t lastZeroOffset = 0;
    uint32_t offset = 0;
    if ((srcLength == 1) && (*src == '0')) {
        if (targetLength < 2) {
                return false;
        }
        target[0] = '0';
        target[1] = '\0';
        return true;
    }
    if (srcLength <= decimals) {
        uint32_t delta = decimals - srcLength;
        if (targetLength < srcLength + 1 + 2 + delta) {
            return false;
        }
        target[offset++] = '0';
        target[offset++] = '.';
        for (uint32_t i = 0; i < delta; i++) {
            target[offset++] = '0';
        }
        startOffset = offset;
        for (uint32_t i = 0; i < srcLength; i++) {
            target[offset++] = src[i];
        }
        target[offset] = '\0';
    } else {
        uint32_t sourceOffset = 0;
        uint32_t delta = srcLength - decimals;
        if (targetLength < srcLength + 1 + 1) {
            return false;
        }
        while (offset < delta) {
            target[offset++] = src[sourceOffset++];
        }
        if (decimals != 0) {
            target[offset++] = '.';
        }
        startOffset = offset;
        while (sourceOffset < srcLength) {
            target[offset++] = src[sourceOffset++];
        }
	target[offset] = '\0';
    }
    for (uint32_t i = startOffset; i < offset; i++) {
        if (target[i] == '0') {
            if (lastZeroOffset == 0) {
                lastZeroOffset = i;
            }
        } else {
            lastZeroOffset = 0;
        }
    }
    if (lastZeroOffset != 0) {
        target[lastZeroOffset] = '\0';
        if (target[lastZeroOffset - 1] == '.') {
                target[lastZeroOffset - 1] = '\0';
        }
    }
    return true;
}
