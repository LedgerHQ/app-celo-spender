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

#include "ethUstream.h"
#include "rlp.h"


#ifdef TESTING
#define PRINTF(...)
#endif

/**
 * @brief Reads a byte from the transaction buffer.
 *
 * @param[in,out] context The transaction context.
 * @param[out] byte The byte read from the buffer.
 *
 * @return 0 if successful, -1 if underflow occurs.
 */
static int readTxByte(txContext_t *context, uint8_t *byte) {
    uint8_t data;

    if (context->commandLength < 1) {
        PRINTF("readTxByte Underflow\n");
        return -1;
    }
    data = *context->workBuffer;
    context->workBuffer++;
    context->commandLength--;
    if (context->processingField) {
        context->currentFieldPos++;
    }
#ifndef TESTING
    if (!(context->processingField && context->fieldSingleByte)) {
        CX_THROW(cx_hash_no_throw((cx_hash_t*)context->sha3, 0, &data, 1, NULL, 0));
    }
#endif
    if (byte) {
        *byte = data;
    }
    return 0;
}

/**
 * @brief Copies data from the transaction buffer to the output buffer.
 *
 * @param[in,out] context The transaction context.
 * @param[out] out The output buffer.
 * @param[in] length The length of data to be copied.
 *
 * @return 0 if successful, -1 if underflow occurs.
 */
int copyTxData(txContext_t *context, uint8_t *out, size_t length)  {
    if (context->commandLength < length) {
        PRINTF("copyTxData Underflow\n");
        return -1;
    }
    if (out != NULL) {
        memcpy(out, context->workBuffer, length);
    }
#ifndef TESTING
    if (!(context->processingField && context->fieldSingleByte)) {
        CX_THROW(cx_hash_no_throw((cx_hash_t*)context->sha3, 0, context->workBuffer, length, NULL, 0));
    }
#endif
    context->workBuffer += length;
    context->commandLength -= length;
    if (context->processingField) {
        context->currentFieldPos += length;
    }
    return 0;
}

/**
 * @brief Processes the RLP_CONTENT field.
 *
 * @param[in,out] context The transaction context.
 *
 * @return false if successful, true if an error occurs.
 */
static bool processContent(txContext_t *context) {
    if (context == NULL) {
        PRINTF("Context pointer is NULL\n");
        return true;
    }
    // Keep the full length for sanity checks, move to the next field
    if (!context->currentFieldIsList) {
        PRINTF("Invalid type for RLP_CONTENT\n");
        return true;
    }
    context->dataLength = context->currentFieldLength;
    context->currentField++;
    context->processingField = false;
    return false;
}

/**
 * @brief Processes the RLP_ACCESS_LIST field.
 *
 * @param[in,out] context The transaction context.
 *
 * @return false if successful, true if an error occurs.
 */
static bool processAccessList(txContext_t *context) {
    if (context == NULL) {
        PRINTF("Context pointer is NULL\n");
        return true;
    }
    if (!context->currentFieldIsList) {
        PRINTF("Invalid type for RLP_ACCESS_LIST\n");
        return true;
    }
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t copySize =
            MIN(context->commandLength, context->currentFieldLength - context->currentFieldPos);
        copyTxData(context, NULL, copySize);
    }
    if (context->currentFieldPos == context->currentFieldLength) {
        context->currentField++;
        context->processingField = false;
    }
    return false;
}

/**
 * @brief Processes the RLP_TYPE field.
 *
 * @param[in,out] context The transaction context.
 *
 * @return false if successful, true if an error occurs.
 */
static bool processType(txContext_t *context) {
    if (context == NULL) {
        PRINTF("Context pointer is NULL\n");
        return true;
    }
    if (context->currentFieldIsList) {
        PRINTF("Invalid type for RLP_TYPE\n");
        return true;
    }
    if (context->currentFieldLength > MAX_INT256) {
        PRINTF("Invalid length for RLP_TYPE\n");
        return true;
    }
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t copySize =
            (context->commandLength <
                     ((context->currentFieldLength - context->currentFieldPos))
                 ? context->commandLength
                 : context->currentFieldLength - context->currentFieldPos);
        if (copyTxData(context, NULL, copySize)) {
            return true;
        }
    }
    if (context->currentFieldPos == context->currentFieldLength) {
        context->currentField++;
        context->processingField = false;
    }
    return false;
}

/**
 * @brief Processes the RLP_NONCE field.
 *
 * @param[in,out] context The transaction context.
 *
 * @return false if successful, true if an error occurs.
 */
static bool processNonce(txContext_t *context) {
    if (context == NULL) {
        PRINTF("Context pointer is NULL\n");
        return true;
    }
    if (context->currentFieldIsList) {
        PRINTF("Invalid type for RLP_NONCE\n");
        return true;
    }
    if (context->currentFieldLength > MAX_INT256) {
        PRINTF("Invalid length for RLP_NONCE\n");
        return true;
    }
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t copySize =
            (context->commandLength <
                     ((context->currentFieldLength - context->currentFieldPos))
                 ? context->commandLength
                 : context->currentFieldLength - context->currentFieldPos);
        if (copyTxData(context, NULL, copySize)) {
            return true;
        }
    }
    if (context->currentFieldPos == context->currentFieldLength) {
        context->currentField++;
        context->processingField = false;
    }
    return false;
}

/**
 * @brief Processes the RLP_STARTGAS field.
 *
 * @param[in,out] context The transaction context.
 *
 * @return false if successful, true if an error occurs.
 */
static bool processStartGas(txContext_t *context) {
    if (context == NULL) {
        PRINTF("Context pointer is NULL\n");
        return true;
    }
    if (context->currentFieldIsList) {
        PRINTF("Invalid type for RLP_STARTGAS\n");
        return true;
    }
    if (context->currentFieldLength > MAX_INT256) {
        PRINTF("Invalid length for RLP_STARTGAS %d\n",
                      context->currentFieldLength);
        return true;
    }
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t copySize =
            (context->commandLength <
                     ((context->currentFieldLength - context->currentFieldPos))
                 ? context->commandLength
                 : context->currentFieldLength - context->currentFieldPos);
        if (copyTxData(context, context->content->startgas.value + context->currentFieldPos, copySize)) {
            return true;
        }
    }
    if (context->currentFieldPos == context->currentFieldLength) {
        context->content->startgas.length = context->currentFieldLength;
        context->currentField++;
        context->processingField = false;
    }
    return false;
}

/**
 * @brief Processes the RLP_GASPRICE field.
 *
 * @param[in,out] context The transaction context.
 *
 * @return false if successful, true if an error occurs.
 */
static bool processGasprice(txContext_t *context) {
    if (context == NULL) {
        PRINTF("Context pointer is NULL\n");
        return true;
    }
    if (context->currentFieldIsList) {
        PRINTF("Invalid type for RLP_GASPRICE\n");
        return true;
    }
    if (context->currentFieldLength > MAX_INT256) {
        PRINTF("Invalid length for RLP_GASPRICE\n");
        return true;
    }
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t copySize =
            (context->commandLength <
                     ((context->currentFieldLength - context->currentFieldPos))
                 ? context->commandLength
                 : context->currentFieldLength - context->currentFieldPos);
        if (copyTxData(context, context->content->gasprice.value + context->currentFieldPos, copySize)) {
            return true;
        }
    }
    if (context->currentFieldPos == context->currentFieldLength) {
        context->content->gasprice.length = context->currentFieldLength;
        context->currentField++;
        context->processingField = false;
    }
    return false;
}

/**
 * @brief Processes the RLP_FEECURRENCY field.
 *
 * @param[in,out] context The transaction context.
 *
 * @return false if successful, true if an error occurs.
 */
static bool processFeeCurrency(txContext_t *context) {
    if (context == NULL) {
        PRINTF("Context pointer is NULL\n");
        return true;
    }
    if (context->currentFieldIsList) {
        PRINTF("Invalid type for RLP_FEECURRENCY\n");
        return true;
    }
    if (context->currentFieldLength > MAX_ADDRESS) {
        PRINTF("Invalid length for RLP_FEECURRENCY\n");
        return true;
    }
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t copySize =
            (context->commandLength <
                     ((context->currentFieldLength - context->currentFieldPos))
                 ? context->commandLength
                 : context->currentFieldLength - context->currentFieldPos);
        if (copyTxData(context, context->content->feeCurrency + context->currentFieldPos, copySize)) {
            return true;
        }
    }
    if (context->currentFieldPos == context->currentFieldLength) {
        context->content->feeCurrencyLength = context->currentFieldLength;    
        context->currentField++;
        context->processingField = false;
    }
    return false;
}

/**
 * @brief Processes the RLP_VALUE field.
 *
 * @param[in,out] context The transaction context.
 *
 * @return false if successful, true if an error occurs.
 */
static bool processValue(txContext_t *context) {
    if (context == NULL) {
        PRINTF("Context pointer is NULL\n");
        return true;
    }
    if (context->currentFieldIsList) {
        PRINTF("Invalid type for RLP_VALUE\n");
        return true;
    }
    if (context->currentFieldLength > MAX_INT256) {
        PRINTF("Invalid length for RLP_VALUE\n");
        return true;
    }
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t copySize =
            (context->commandLength <
                     ((context->currentFieldLength - context->currentFieldPos))
                 ? context->commandLength
                 : context->currentFieldLength - context->currentFieldPos);
        if (copyTxData(context, context->content->value.value + context->currentFieldPos, copySize)) {
            return true;
        }
    }
    if (context->currentFieldPos == context->currentFieldLength) {
        context->content->value.length = context->currentFieldLength;
        context->currentField++;
        context->processingField = false;
    }
    return false;
}

/**
 * @brief Processes the RLP_TO field.
 *
 * @param[in,out] context The transaction context.
 *
 * @return false if successful, true if an error occurs.
 */
static bool processTo(txContext_t *context) {
    if (context == NULL) {
        PRINTF("Context pointer is NULL\n");
        return true;
    }
    if (context->currentFieldIsList) {
        PRINTF("Invalid type for RLP_TO\n");
        return true;
    }
    if (context->currentFieldLength != 0 && context->currentFieldLength != MAX_ADDRESS) {
        PRINTF("Invalid length for RLP_TO\n");
        return true;
    }
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t copySize =
            (context->commandLength <
                     ((context->currentFieldLength - context->currentFieldPos))
                 ? context->commandLength
                 : context->currentFieldLength - context->currentFieldPos);
        if (copyTxData(context, context->content->destination + context->currentFieldPos, copySize)) {
            return true;
        }
    }
    if (context->currentFieldPos == context->currentFieldLength) {
        context->content->destinationLength = context->currentFieldLength;
        context->currentField++;
        context->processingField = false;
    }
    return false;
}

/**
 * @brief Processes the RLP_DATA field.
 *
 * @param[in,out] context The transaction context.
 *
 * @return false if successful, true if an error occurs.
 */
static bool processData(txContext_t *context) {
    if (context == NULL) {
        PRINTF("Context pointer is NULL\n");
        return true;
    }
    if (context->currentFieldIsList) {
        PRINTF("Invalid type for RLP_DATA\n");
        return true;
    }
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t copySize =
            (context->commandLength <
                     ((context->currentFieldLength - context->currentFieldPos))
                 ? context->commandLength
                 : context->currentFieldLength - context->currentFieldPos);
        if (copyTxData(context, NULL, copySize)) {
            return true;
        }
    }
    if (context->currentFieldPos == context->currentFieldLength) {
        context->currentField++;
        context->processingField = false;
    }
    return false;
}

/**
 * @brief Processes a discarded field.
 *
 * @param[in,out] context The transaction context.
 *
 * @return false if successful, true if an error occurs.
 */
static bool processAndDiscard(txContext_t *context) {
    if (context == NULL) {
        PRINTF("Context pointer is NULL\n");
        return true;
    }
    if (context->currentFieldIsList) {
        PRINTF("Invalid type for Discarded field\n");
        return true;
    }
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t copySize =
            MIN(context->commandLength, context->currentFieldLength - context->currentFieldPos);
        copyTxData(context, NULL, copySize);
    }
    if (context->currentFieldPos == context->currentFieldLength) {
        context->currentField++;
        context->processingField = false;
    }
    return false;
}
#define NUM_CHAIN_IDS 3
                                                        // Mainnet, Alfajores, Baklava,
static const uint16_t AUTHORIZED_CHAIN_IDS[NUM_CHAIN_IDS] = {42220, 44787, 17323};

/**
 * @brief Checks if the given chain ID is authorized.
 *
 * @param[in] chainID The chain ID to check.
 *
 * @return 1 if authorized, 0 otherwise.
 */
static int isChainIDAuthorized(uint8_t chainID[4]) {
    if(chainID == NULL) {
        return 0;
    }
    uint16_t chainIDInt = (chainID[0] << 8) | chainID[1];
    for (int i = 0; i < NUM_CHAIN_IDS; i++) {
        if (chainIDInt == AUTHORIZED_CHAIN_IDS[i]) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Processes the RLP_V field.
 *
 * @param[in,out] context The transaction context.
 *
 * @return false if successful, true if an error occurs.
 */
static bool processV(txContext_t *context) {
    if (context == NULL) {
        PRINTF("Context pointer is NULL\n");
        return true;
    }
    if (context->currentFieldIsList) {
        PRINTF("Invalid type for RLP_V\n");
        return true;
    }
    if (context->currentFieldLength > MAX_V) {
        PRINTF("Invalid length for RLP_V\n");
        return true;
    }
    if (context->currentFieldPos < context->currentFieldLength) {
        uint32_t copySize =
            (context->commandLength <
                     ((context->currentFieldLength - context->currentFieldPos))
                 ? context->commandLength
                 : context->currentFieldLength - context->currentFieldPos);
        if (copyTxData(context, context->content->v + context->currentFieldPos, copySize)) {
            return true;
        }
        if (!isChainIDAuthorized(context->content->v)) {
            PRINTF("ChainID not authorized\n");
            return true;
        }
    }
    if (context->currentFieldPos == context->currentFieldLength) {
        context->content->vLength = context->currentFieldLength;
        context->currentField++;
        context->processingField = false;
    }
    return false;
}

/**
 * @brief Processes a CIP64 transaction.
 *
 * @param[in,out] context The transaction context.
 *
 * @return false if the transaction is processed successfully, true otherwise.
 */
static bool processCIP64Tx(txContext_t *context) {
    if (context == NULL) {
        PRINTF("Context pointer is NULL\n");
        return true;
    }
    switch (context->currentField) {
        case CIP64_RLP_CONTENT: {
            if(processContent(context)) {
                return true;
            }
            if ((context->processingFlags & TX_FLAG_TYPE) == 0) {
                context->currentField++;
            }
            return false;
        }
        // This gets hit only by Wanchain
        case CIP64_RLP_TYPE: {
            return processType(context);
        }
        case CIP64_RLP_CHAINID: {
            return processV(context);
        }
        case CIP64_RLP_NONCE: {
            return processNonce(context);
        }
        case CIP64_RLP_MAX_PRIORITY_FEE_PER_GAS: {
            return processAndDiscard(context);
        }
        case CIP64_RLP_MAX_FEE_PER_GAS: {
            return processGasprice(context);
        }
        case CIP64_RLP_GASLIMIT: {
            return processStartGas(context);
        }
        case CIP64_RLP_TO: {
            return processTo(context);
        }
        case CIP64_RLP_VALUE: {
            return processValue(context);
        }
        case CIP64_RLP_DATA: {
            return processData(context);
        }
        case CIP64_RLP_ACCESS_LIST: {
            return processAccessList(context);
        }
        case CIP64_RLP_FEECURRENCY: {
            return processFeeCurrency(context);
        }
        default:
            PRINTF("Invalid RLP decoder context\n");
            return true;
    }
}

/**
 * @brief Processes an EIP1559 transaction.
 *
 * @param[in,out] context The transaction context.
 *
 * @return false if the transaction is processed successfully, true otherwise.
 */
static bool processEIP1559Tx(txContext_t *context) {
    if (context == NULL) {
        PRINTF("Context pointer is NULL\n");
        return true;
    }
    switch (context->currentField) {
        case EIP1559_RLP_CONTENT: {
            if(processContent(context)) {
                return true;
            }
            if ((context->processingFlags & TX_FLAG_TYPE) == 0) {
                context->currentField++;
            }
            return false;
        }
        case EIP1559_RLP_TYPE: {
            return processType(context);
        }
        case EIP1559_RLP_CHAINID: {
            return processV(context);
        }
        case EIP1559_RLP_NONCE: {
            return processNonce(context);
        }
        case EIP1559_RLP_MAX_FEE_PER_GAS: {
            return processGasprice(context);
        }
        case EIP1559_RLP_GASLIMIT: {
            return processStartGas(context);
        }
        case EIP1559_RLP_TO: {
            return processTo(context);
        }
        case EIP1559_RLP_VALUE: {
            return processValue(context);
        }
        case EIP1559_RLP_DATA: {
            return processData(context);
        }
        case EIP1559_RLP_ACCESS_LIST: {
            return processAccessList(context);
        }
        case EIP1559_RLP_MAX_PRIORITY_FEE_PER_GAS: {
            return processAndDiscard(context);
        }
        default:
            PRINTF("Invalid RLP decoder context\n");
            return true;
    }
}

/**
 * @brief Parses the RLP buffer and decodes the current field.
 *
 * @param[in,out] context The transaction context.
 *
 * @return USTREAM_PROCESSING if the RLP buffer is being processed, USTREAM_FAULT if there is an error, USTREAM_PROCESSING if more data is needed.
 */
static parserStatus_e parseRLP(txContext_t *context) {
    if (context == NULL) {
        PRINTF("Context pointer is NULL\n");
        return USTREAM_FAULT;
    }
    bool canDecode = false;
    uint32_t offset;
    while (context->commandLength != 0) {
        bool valid;
        // Feed the RLP buffer until the length can be decoded
        uint8_t byte;
        if (readTxByte(context, &byte)) {
            return USTREAM_FAULT;
        }
        context->rlpBuffer[context->rlpBufferPos++] = byte;
        if (rlpCanDecode(context->rlpBuffer, context->rlpBufferPos, &valid)) {
            // Can decode now, if valid
            if (!valid) {
                PRINTF("RLP pre-decode error\n");
                return USTREAM_FAULT;
            }
            canDecode = true;
            break;
        }
        // Cannot decode yet
        // Sanity check
        if (context->rlpBufferPos == sizeof(context->rlpBuffer)) {
            PRINTF("RLP pre-decode logic error\n");
            return USTREAM_FAULT;
        }
    }
    if (!canDecode) {
        return USTREAM_PROCESSING;
    }
    // Ready to process this field
    if (!rlpDecodeLength(context->rlpBuffer, context->rlpBufferPos,
                            &context->currentFieldLength, &offset,
                            &context->currentFieldIsList)) {
        PRINTF("RLP decode error\n");
        return USTREAM_FAULT;
    }
    if (offset == 0) {
        // Hack for single byte, self encoded
        context->workBuffer--;
        context->commandLength++;
        context->fieldSingleByte = true;
    } else {
        context->fieldSingleByte = false;
    }
    context->currentFieldPos = 0;
    context->rlpBufferPos = 0;
    context->processingField = true;
    return USTREAM_PROCESSING;
}

/**
 * @brief Processes the transaction fields based on the transaction type.
 *
 * @param[in,out] context The transaction context.
 *
 * @return USTREAM_FINISHED if the transaction processing is complete, USTREAM_PROCESSING if more data is needed, USTREAM_FAULT if there is an error.
 */
static parserStatus_e processTxInternal(txContext_t *context) {
    if (context == NULL) {
        PRINTF("Context pointer is NULL\n");
        return USTREAM_FAULT;
    }
    for (;;) {
        customStatus_e customStatus = CUSTOM_NOT_HANDLED;
        // EIP 155 style transaction
        if (PARSING_IS_DONE(context)) {
            PRINTF("Parsing done\n");
            return USTREAM_FINISHED;
        }
        // Old style transaction
        if ((context->txType == CELO_LEGACY && context->currentField == CELO_LEGACY_RLP_V) && (context->commandLength == 0)) {
            context->content->vLength = 0;
            // We don't want to support old style transactions. We treat an empty V as a false positive
            // - data ended exactly on the APDU boundary, and so we tell the processing to continue.
            return USTREAM_PROCESSING;
        }
        if (context->commandLength == 0) {
            return USTREAM_PROCESSING;
        }
        if (!context->processingField) {
            parserStatus_e status = parseRLP(context);
            if(status != USTREAM_PROCESSING) {
                return status;
            }
        }
        if (context->customProcessor != NULL) {
            customStatus = context->customProcessor(context);
            switch(customStatus) {
                case CUSTOM_NOT_HANDLED:
                case CUSTOM_HANDLED:
                    break;
                case CUSTOM_SUSPENDED:
                    return USTREAM_SUSPENDED;
                case CUSTOM_FAULT:
                    PRINTF("Custom processor aborted\n");
                    return USTREAM_FAULT;
                default:
                    PRINTF("Unhandled custom processor status\n");
                    return USTREAM_FAULT;
            }
        }
        if (customStatus == CUSTOM_NOT_HANDLED) {
            switch(context->txType) {
                case EIP1559:
                    if (processEIP1559Tx(context)) {
                        return USTREAM_FAULT;
                    }
                    break;
                case CIP64:
                    if (processCIP64Tx(context)) {
                        return USTREAM_FAULT;
                    }
                    break;
                default:
                    PRINTF("Transaction type %d not supported\n", context->txType);
                    return USTREAM_FAULT;
            }
        }
    }
}

/**
 * @brief Processes the transaction.
 *
 * @param[in,out] context The transaction context.
 * @param[in] buffer The input buffer containing the transaction data.
 * @param[in] length The length of the input buffer.
 *
 * @return USTREAM_FINISHED if the transaction processing is complete, USTREAM_PROCESSING if more data is needed, USTREAM_FAULT if there is an error.
 */
parserStatus_e processTx(txContext_t *context, const uint8_t *buffer, size_t length) {
    if (context == NULL) {
        PRINTF("Context pointer is NULL\n");
        return USTREAM_FAULT;
    }
    context->workBuffer = buffer;
    context->commandLength = length;
    return processTxInternal(context);
}

/**
 * @brief Continues processing the transaction.
 *
 * @param[in,out] context The transaction context.
 *
 * @return USTREAM_FINISHED if the transaction processing is complete, USTREAM_PROCESSING if more data is needed, USTREAM_FAULT if there is an error.
 */
parserStatus_e continueTx(txContext_t *context) {
    if (context == NULL) {
        PRINTF("Context pointer is NULL\n");
        return USTREAM_FAULT;
    }
    return processTxInternal(context);
}

/**
 * @brief Initializes the transaction context.
 *
 * @param[in,out] context The transaction context.
 * @param[in] sha3 The SHA3 context.
 * @param[in] content The transaction content.
 * @param[in] customProcessor The custom processor function.
 * @param[in] extra Additional data for the custom processor.
 */
void initTx(txContext_t *context, cx_sha3_t *sha3, txContent_t *content, ustreamProcess_t customProcessor, void *extra) {
    memset(context, 0, sizeof(txContext_t));
    context->sha3 = sha3;
    context->content = content;
    context->customProcessor = customProcessor;
    context->extra = extra;
    context->currentField = CELO_LEGACY_RLP_CONTENT;
#ifndef TESTING
    CX_THROW(cx_keccak_init_no_throw(context->sha3, 256));
#endif
}
