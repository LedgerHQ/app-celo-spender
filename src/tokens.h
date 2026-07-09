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

#ifndef _TOKENS_H_
#define _TOKENS_H_

#include <stdint.h>

// EVM address size in raw bytes (20 bytes = 160 bits).
#define EVM_ADDRESS_LEN 20

// Max ticker length + null terminator. Must stay in sync with MAX_TICKER_LEN in globals.h
// (enforced by a _Static_assert there).
#define TOKEN_TICKER_BUF_LEN 51

typedef struct tokenDefinition_t {
    uint8_t address[EVM_ADDRESS_LEN];
    char ticker[TOKEN_TICKER_BUF_LEN];
    uint8_t decimals;
} tokenDefinition_t;

#endif /* _TOKENS_H_ */
