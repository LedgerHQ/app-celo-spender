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

#ifndef _ETHUTILS_H_
#define _ETHUTILS_H_

#include <stdint.h>
#include <stdbool.h>

#include "cx.h"

void getEthAddressFromKey(const cx_ecfp_public_key_t *publicKey, uint8_t *out,
                                cx_sha3_t *sha3Context);

void getEthAddressStringFromKey(const cx_ecfp_public_key_t *publicKey, char *out,
                                cx_sha3_t *sha3Context);

void getEthAddressStringFromBinary(const uint8_t *address, char *out,
                                   cx_sha3_t *sha3Context);

bool adjustDecimals(const char *src, size_t srcLength, char *target,
                    size_t targetLength, uint8_t decimals);

#endif /* _ETHUTILS_H_ */
