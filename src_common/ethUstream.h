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

#ifndef _ETHUSTREAM_H_
#define _ETHUSTREAM_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef TESTING
typedef void *cx_sha3_t;
#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif
#else
#include "os.h"
#include "cx.h"
#endif

#define MAX_INT256  32
#define MAX_ADDRESS 20
#define MAX_V       4

#define TX_FLAG_TYPE 0x01

// First variant of every Tx enum.
#define RLP_NONE 0

#define PARSING_IS_DONE(ctx)                                          \
    ((ctx->txType == CIP64 && ctx->currentField == CIP64_RLP_DONE) || \
     (ctx->txType == EIP1559 && ctx->currentField == EIP1559_RLP_DONE))

typedef enum rlpCIP64TxField_e {
    CIP64_RLP_NONE = RLP_NONE,
    CIP64_RLP_CONTENT,
    CIP64_RLP_TYPE,
    CIP64_RLP_CHAINID,
    CIP64_RLP_NONCE,
    CIP64_RLP_MAX_PRIORITY_FEE_PER_GAS,
    CIP64_RLP_MAX_FEE_PER_GAS,
    CIP64_RLP_GASLIMIT,
    CIP64_RLP_TO,
    CIP64_RLP_VALUE,
    CIP64_RLP_DATA,
    CIP64_RLP_ACCESS_LIST,
    CIP64_RLP_FEECURRENCY,
    CIP64_RLP_DONE
} rlpCIP64TxField_e;

typedef enum rlpEIP1559TxField_e {
    EIP1559_RLP_NONE = RLP_NONE,
    EIP1559_RLP_CONTENT,
    EIP1559_RLP_TYPE,
    EIP1559_RLP_CHAINID,
    EIP1559_RLP_NONCE,
    EIP1559_RLP_MAX_PRIORITY_FEE_PER_GAS,
    EIP1559_RLP_MAX_FEE_PER_GAS,
    EIP1559_RLP_GASLIMIT,
    EIP1559_RLP_TO,
    EIP1559_RLP_VALUE,
    EIP1559_RLP_DATA,
    EIP1559_RLP_ACCESS_LIST,
    EIP1559_RLP_DONE
} rlpEIP1559TxField_e;

struct txContext_t;

// Valid transaction types
typedef enum txType_e {
    EIP1559 = 0x02,
    CIP64 = 0x7b,  // 123
} txType_e;

typedef enum customStatus_e {
    CUSTOM_NOT_HANDLED,
    CUSTOM_HANDLED,
    CUSTOM_SUSPENDED,
    CUSTOM_FAULT
} customStatus_e;

typedef customStatus_e (*ustreamProcess_t)(struct txContext_t *context);

typedef enum parserStatus_e {
    USTREAM_PROCESSING,
    USTREAM_SUSPENDED,
    USTREAM_FINISHED,
    USTREAM_FAULT
} parserStatus_e;

typedef struct txInt256_t {
    uint8_t value[MAX_INT256];
    uint8_t length;
} txInt256_t;

typedef struct txContent_t {
    txInt256_t gasprice;
    txInt256_t startgas;
    txInt256_t value;
    txInt256_t gatewayFee;
    uint8_t destination[MAX_ADDRESS];
    uint8_t destinationLength;
    uint8_t gatewayDestination[MAX_ADDRESS];
    uint8_t gatewayDestinationLength;
    uint8_t feeCurrency[20];
    uint8_t feeCurrencyLength;
    uint8_t v[MAX_V];
    uint8_t vLength;
    bool dataPresent;
} txContent_t;

typedef struct txContext_t {
    uint8_t currentField;
    cx_sha3_t *sha3;
    uint32_t currentFieldLength;
    uint32_t currentFieldPos;
    bool currentFieldIsList;
    bool processingField;
    bool fieldSingleByte;
    uint32_t dataLength;
    uint8_t rlpBuffer[5];
    uint32_t rlpBufferPos;
    const uint8_t *workBuffer;
    uint32_t commandLength;
    uint32_t processingFlags;
    ustreamProcess_t customProcessor;
    txContent_t *content;
    void *extra;
    uint8_t txType;
} txContext_t;

void initTx(txContext_t *context,
            cx_sha3_t *sha3,
            txContent_t *content,
            ustreamProcess_t customProcessor,
            void *extra);
parserStatus_e processTx(txContext_t *context, const uint8_t *buffer, size_t length);
parserStatus_e continueTx(txContext_t *context);
int copyTxData(txContext_t *context, uint8_t *out, size_t length);

#endif /* _ETHUSTREAM_H_ */
