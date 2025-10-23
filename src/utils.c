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

#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "ethUstream.h"
#include "uint256.h"

static const char hex_digits[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

void array_hexstr(char *strbuf, const void *bin, size_t len) {
    while (len--) {
        *strbuf++ = hex_digits[((*((char *)bin)) >> 4) & 0xF];
        *strbuf++ = hex_digits[(*((char *)bin)) & 0xF];
        bin = (const void *)((unsigned int)bin + 1);
    }
    *strbuf = 0; // EOS
}

void convertUint256BE(const uint8_t *data, size_t length, uint256_t *target) {
    uint8_t tmp[32];
    memset(tmp, 0, 32);
    memcpy(tmp + 32 - length, data, length);
    readu256BE(tmp, target);
}

uint32_t getV(txContent_t *txContent) {
    uint32_t v = 0;
    if (txContent->vLength == 1) {
      v = txContent->v[0];
    }
    else
    if (txContent->vLength == 2) {
      v = (txContent->v[0] << 8) | txContent->v[1];
    }
    else
    if (txContent->vLength == 3) {
      v = (txContent->v[0] << 16) | (txContent->v[1] << 8) | txContent->v[2];
    }
    else
    if (txContent->vLength == 4) {
      v = (txContent->v[0] << 24) | (txContent->v[1] << 16) |
          (txContent->v[2] << 8) | txContent->v[3];
    }
    else
    if (txContent->vLength != 0) {
        PRINTF("Unexpected v format\n");
        THROW(EXCEPTION);
    }
    return v;
}

int _strcasecmp(const char *s1, const char *s2) {
    const unsigned char *p1 = (const unsigned char *) s1;
    const unsigned char *p2 = (const unsigned char *) s2;
    int result = 0;
    if (p1 == p2) return 0;
    while ((result = toupper(*p1) - toupper(*p2++)) == 0)
        if (*p1++ == '\0') break;
    return result;
}