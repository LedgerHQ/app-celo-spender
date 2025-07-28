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

#include <ctype.h>
#include <string.h>

#include "ethUstream.h"
#include "ethUtils.h"
#include "globals.h"
#include "format.h"
#include "utils.h"

static const char hex_digits[] =
    {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

void array_hexstr(char *strbuf, const void *bin, size_t len) {
    while (len--) {
        *strbuf++ = hex_digits[((*((char *) bin)) >> 4) & 0xF];
        *strbuf++ = hex_digits[(*((char *) bin)) & 0xF];
        bin = (const void *) ((unsigned int) bin + 1);
    }
    *strbuf = 0;  // EOS
}

uint32_t getV(txContent_t *txContent) {
    uint32_t v = 0;
    if (txContent->vLength == 1) {
        v = txContent->v[0];
    } else if (txContent->vLength == 2) {
        v = (txContent->v[0] << 8) | txContent->v[1];
    } else if (txContent->vLength == 3) {
        v = (txContent->v[0] << 16) | (txContent->v[1] << 8) | txContent->v[2];
    } else if (txContent->vLength == 4) {
        v = (txContent->v[0] << 24) | (txContent->v[1] << 16) | (txContent->v[2] << 8) |
            txContent->v[3];
    } else if (txContent->vLength != 0) {
        PRINTF("Unexpected v format\n");
        THROW(EXCEPTION);
    }
    return v;
}

int parse_bip32_path(bip32Path_t *derivationPath, const uint8_t *input, size_t len) {
    uint8_t path_length;
    uint8_t offset = 0;

    if (len == 0 || input == NULL) {
        return -1;
    }
    path_length = input[offset];
    if (path_length < 1 || path_length > MAX_BIP32_PATH) {
        return -1;
    }
    if (len < 1 + path_length * sizeof(uint32_t)) {
        return -1;
    }

    offset++;
    input += offset;
    for (size_t i = 0; i < path_length; i++) {
        derivationPath->path[i] =
            (input[0] << 24u) | (input[1] << 16u) | (input[2] << 8u) | input[3];
        input += 4;
        offset += 4;
    }
    derivationPath->len = path_length;
    return offset;
}

int array_bytes_string(char *out, size_t outl, const void *value, size_t len) {
    if (outl <= 2) {
        // Need at least '0x' and 1 digit
        return -1;
    }
    if (strlcpy(out, "0x", outl) != 2) {
        goto err;
    }
    if (format_hex(value, len, out + 2, outl - 2) < 0) {
        goto err;
    }
    return 0;
err:
    *out = '\0';
    return -1;
}

int allzeroes(const void *buf, size_t n) {
    uint8_t *p = (uint8_t *) buf;
    for (size_t i = 0; i < n; ++i) {
        if (p[i]) {
            return 0;
        }
    }
    return 1;
}

uint64_t u64_from_BE(const uint8_t *in, uint8_t size) {
    uint8_t i = 0;
    uint64_t res = 0;

    while (i < size && i < sizeof(res)) {
        res <<= 8;
        res |= in[i];
        i++;
    }

    return res;
}

bool u64_to_string(uint64_t src, char *dst, uint8_t dst_size) {
    // Copy the numbers in ASCII format.
    uint8_t i = 0;
    do {
        // Checking `i + 1` to make sure we have enough space for '\0'.
        if (i + 1 >= dst_size) {
            return false;
        }
        dst[i] = src % 10 + '0';
        src /= 10;
        i++;
    } while (src);

    // Null terminate string
    dst[i] = '\0';

    // Revert the string
    i--;
    uint8_t j = 0;
    while (j < i) {
        char tmp = dst[i];
        dst[i] = dst[j];
        dst[j] = tmp;
        i--;
        j++;
    }
    return true;
}

bool new_getEthAddressStringFromBinary(uint8_t *address,
                                       char out[static(ADDRESS_LENGTH * 2) + 1],
                                       uint64_t chainId) {
    // save some precious stack space
    union locals_union {
        uint8_t hashChecksum[INT256_LENGTH];
        uint8_t tmp[51];
    } locals_union;

    uint8_t i;
    bool eip1191 = false;
    uint32_t offset = 0;
    switch (chainId) {
        case 30:
        case 31:
            eip1191 = true;
            break;
    }
    if (eip1191) {
        if (!u64_to_string(chainId, (char *) locals_union.tmp, sizeof(locals_union.tmp))) {
            return false;
        }
        offset = strnlen((char *) locals_union.tmp, sizeof(locals_union.tmp));
        strlcat((char *) locals_union.tmp + offset, "0x", sizeof(locals_union.tmp) - offset);
        offset = strnlen((char *) locals_union.tmp, sizeof(locals_union.tmp));
    }
    for (i = 0; i < 20; i++) {
        uint8_t digit = address[i];
        locals_union.tmp[offset + 2 * i] = HEXDIGITS[(digit >> 4) & 0x0f];
        locals_union.tmp[offset + 2 * i + 1] = HEXDIGITS[digit & 0x0f];
    }
    if (cx_keccak_256_hash(locals_union.tmp, offset + 40, locals_union.hashChecksum) != CX_OK) {
        return false;
    }

    for (i = 0; i < 40; i++) {
        uint8_t digit = address[i / 2];
        if ((i % 2) == 0) {
            digit = (digit >> 4) & 0x0f;
        } else {
            digit = digit & 0x0f;
        }
        if (digit < 10) {
            out[i] = HEXDIGITS[digit];
        } else {
            int v = (locals_union.hashChecksum[i / 2] >> (4 * (1 - i % 2))) & 0x0f;
            if (v >= 8) {
                out[i] = HEXDIGITS[digit] - 'a' + 'A';
            } else {
                out[i] = HEXDIGITS[digit];
            }
        }
    }
    out[ADDRESS_LENGTH * 2] = '\0';

    return true;
}

bool getEthDisplayableAddress(uint8_t *in, char *out, size_t out_len, uint64_t chainId) {
    if (out_len < 43) {
        strlcpy(out, "ERROR", out_len);
        return false;
    }
    out[0] = '0';
    out[1] = 'x';
    if (!new_getEthAddressStringFromBinary(in, out + 2, chainId)) {
        strlcpy(out, "ERROR", out_len);
        return false;
    }
    return true;
}

bool uint256_to_decimal(const uint8_t *value, size_t value_len, char *out, size_t out_len) {
    if (value_len > INT256_LENGTH) {
        // value len is bigger than INT256_LENGTH ?!
        return false;
    }

    uint16_t n[16] = {0};
    // Copy and right-align the number
    memcpy((uint8_t *) n + INT256_LENGTH - value_len, value, value_len);

    // Special case when value is 0
    if (allzeroes(n, INT256_LENGTH)) {
        if (out_len < 2) {
            // Not enough space to hold "0" and \0.
            return false;
        }
        strlcpy(out, "0", out_len);
        return true;
    }

    uint16_t *p = n;
    for (int i = 0; i < 16; i++) {
        n[i] = __builtin_bswap16(*p++);
    }
    int pos = out_len;
    while (!allzeroes(n, sizeof(n))) {
        if (pos == 0) {
            return false;
        }
        pos -= 1;
        unsigned int carry = 0;
        for (int i = 0; i < 16; i++) {
            int rem = ((carry << 16) | n[i]) % 10;
            n[i] = ((carry << 16) | n[i]) / 10;
            carry = rem;
        }
        out[pos] = '0' + carry;
    }
    memmove(out, out + pos, out_len - pos);
    out[out_len - pos] = 0;
    return true;
}

bool amountToString(const uint8_t *amount,
                    uint8_t amount_size,
                    uint8_t decimals,
                    const char *ticker,
                    char *out_buffer,
                    size_t out_buffer_size) {
    char tmp_buffer[100] = {0};

    if (uint256_to_decimal(amount, amount_size, tmp_buffer, sizeof(tmp_buffer)) == false) {
        return false;
    }

    uint8_t amount_len = strnlen(tmp_buffer, sizeof(tmp_buffer));
    uint8_t ticker_len = strnlen(ticker, MAX_TICKER_LEN);

    if (ticker_len > 0) {
        if (out_buffer_size <= ticker_len + 1) {
            return false;
        }
        memcpy(out_buffer, ticker, ticker_len);
        out_buffer[ticker_len++] = ' ';
    }

    if (adjustDecimals(tmp_buffer,
                       amount_len,
                       out_buffer + ticker_len,
                       out_buffer_size - ticker_len - 1,
                       decimals) == false) {
        return false;
    }

    out_buffer[out_buffer_size - 1] = '\0';
    return true;
}

void getEthAddressFromRawKey(const uint8_t raw_pubkey[static 65],
                             uint8_t out[static ADDRESS_LENGTH]) {
    uint8_t hashAddress[CX_KECCAK_256_SIZE];
    CX_ASSERT(cx_keccak_256_hash(raw_pubkey + 1, 64, hashAddress));
    memmove(out, hashAddress + 12, ADDRESS_LENGTH);
}

void getEthAddressStringFromRawKey(const uint8_t raw_pubkey[static 65],
                                   char out[static(ADDRESS_LENGTH * 2) + 1],
                                   uint64_t chainId) {
    uint8_t hashAddress[CX_KECCAK_256_SIZE];
    CX_ASSERT(cx_keccak_256_hash(raw_pubkey + 1, 64, hashAddress));
    new_getEthAddressStringFromBinary(hashAddress + 12, out, chainId);
}

int ismaxint(uint8_t *buf, int n) {
    for (int i = 0; i < n; ++i) {
        if (buf[i] != 0xff) {
            return 0;
        }
    }
    return 1;
}

void buf_shrink_expand(const uint8_t *src, size_t src_size, uint8_t *dst, size_t dst_size) {
    size_t src_off;
    size_t dst_off;

    if (src_size > dst_size) {
        src_off = src_size - dst_size;
        dst_off = 0;
    } else {
        src_off = 0;
        dst_off = dst_size - src_size;
        explicit_bzero(dst, dst_off);
    }
    memcpy(&dst[dst_off], &src[src_off], dst_size - dst_off);
}

void str_cpy_explicit_trunc(const char *src, size_t src_size, char *dst, size_t dst_size) {
    size_t off;
    const char trunc_marker[] = "...";

    if (src_size < dst_size) {
        memcpy(dst, src, src_size);
        dst[src_size] = '\0';
    } else {
        off = dst_size - sizeof(trunc_marker);
        memcpy(dst, src, off);
        memcpy(&dst[off], trunc_marker, sizeof(trunc_marker));
    }
}

/**
 * @brief Check the name is printable.
 *
 * @param[in] data buffer received
 * @param[in] name Name to check
 * @param[in] len Length of the name
 * @return True/False
 */
bool check_name(const uint8_t *name, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        if (!isprint(name[i])) {
            return false;
        }
    }
    return true;
}

// km: I hard coded the ticker to CELO, this might need an update to account for other fee
// currencies possible instead of determining it with chain id
static void raw_fee_to_string(uint256_t *rawFee, char *displayBuffer, uint32_t displayBufferSize) {
    const char *feeTicker = "CELO";
    uint8_t tickerOffset = 0;
    uint32_t i;

    tostring256(rawFee, 10, (char *) (G_io_apdu_buffer + 100), 100);
    i = 0;
    while (G_io_apdu_buffer[100 + i]) {
        i++;
    }
    adjustDecimals((char *) (G_io_apdu_buffer + 100),
                   i,
                   (char *) G_io_apdu_buffer,
                   100,
                   WEI_TO_ETHER);
    i = 0;
    tickerOffset = 0;
    memset(displayBuffer, 0, displayBufferSize);

    while (feeTicker[tickerOffset]) {
        if ((uint32_t) tickerOffset >= displayBufferSize) {
            break;
        }

        displayBuffer[tickerOffset] = feeTicker[tickerOffset];
        tickerOffset++;
    }
    if ((uint32_t) tickerOffset < displayBufferSize) displayBuffer[tickerOffset++] = ' ';
    while (G_io_apdu_buffer[i]) {
        if ((uint32_t) (tickerOffset) + i >= displayBufferSize) {
            break;
        }
        displayBuffer[tickerOffset + i] = G_io_apdu_buffer[i];
        i++;
    }

    if ((uint32_t) (tickerOffset) + i < displayBufferSize) {
        displayBuffer[tickerOffset + i] = '\0';
    }
}

// Compute the fees, transform it to a string, prepend a ticker to it and copy everything to
// `displayBuffer` output
bool max_transaction_fee_to_string(const txInt256_t *BEGasPrice,
                                   const txInt256_t *BEGasLimit,
                                   char *displayBuffer,
                                   uint32_t displayBufferSize) {
    // Use temporary variables to convert values to uint256_t
    uint256_t gasPrice = {0};
    uint256_t gasLimit = {0};
    // Use temporary variable to store the result of the operation in uint256_t
    uint256_t rawFee = {0};

    PRINTF("Gas price %.*H\n", BEGasPrice->length, BEGasPrice->value);
    PRINTF("Gas limit %.*H\n", BEGasLimit->length, BEGasLimit->value);
    convertUint256BE(BEGasPrice->value, BEGasPrice->length, &gasPrice);
    convertUint256BE(BEGasLimit->value, BEGasLimit->length, &gasLimit);
    if (mul256(&gasPrice, &gasLimit, &rawFee) == false) {
        return false;
    }
    raw_fee_to_string(&rawFee, displayBuffer, displayBufferSize);
    return true;
}

bool is_celo_native_address(const uint8_t *addr) {
    static const uint8_t celo_native_addr[ADDRESS_LENGTH] = {
        0x47, 0x1E, 0xCE, 0x37, 0x50, 0xDA, 0x23, 0x7F, 0x93, 0xB8,
        0xE3, 0x39, 0xC5, 0x36, 0x98, 0x9B, 0x89, 0x78, 0xA4, 0x38};
    return memcmp(addr, celo_native_addr, ADDRESS_LENGTH) == 0;
}