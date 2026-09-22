#include "celo.h"

#include <string.h>

#include "ethUtils.h"
#include "globals.h"
#include "handle_copy_transaction_parameters.h"
#include "os.h"
#include "swap_utils.h"
#include "ui_common.h"
#include "utils.h"

static const uint8_t TOKEN_TRANSFER_ID[] = {0xa9, 0x05, 0x9c, 0xbb};
static const uint8_t TOKEN_TRANSFER_WITH_COMMENT_ID[] = {0xe1, 0xd6, 0xac,
                                                         0xeb};
static const uint8_t LOCK_METHOD_ID[] = {0xf8, 0x3d, 0x08, 0xba};
static const uint8_t VOTE_METHOD_ID[] = {0x58, 0x0d, 0x74, 0x7a};
static const uint8_t ACTIVATE_METHOD_ID[] = {0x1c, 0x5a, 0x9d, 0x9c};
static const uint8_t REVOKE_PENDING_METHOD_ID[] = {0x9d, 0xfb, 0x60, 0x81};
static const uint8_t REVOKE_ACTIVE_METHOD_ID[] = {0x6e, 0x19, 0x84, 0x75};
static const uint8_t UNLOCK_METHOD_ID[] = {0x61, 0x98, 0xe3, 0x39};
static const uint8_t WITHDRAW_METHOD_ID[] = {0x2e, 0x1a, 0x7d, 0x4d};
static const uint8_t RELOCK_METHOD_ID[] = {0xb2, 0xfb, 0x30, 0xcb};
static const uint8_t CREATE_ACCOUNT_METHOD_ID[] = {0x9d, 0xca, 0x36, 0x2f};

void io_seproxyhal_send_status(uint32_t sw) {
    G_io_apdu_buffer[0] = ((sw >> 8) & 0xff);
    G_io_apdu_buffer[1] = (sw & 0xff);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
}

void format_signature_out(const uint8_t* signature) {
    memset(G_io_apdu_buffer + 1, 0x00, 64);
    uint8_t offset = 1;
    uint8_t xoffset = 4;  // point to r value
    // copy r
    uint8_t xlength = signature[xoffset - 1];
    if (xlength == 33) {
        xlength = 32;
        xoffset++;
    }
    memmove(G_io_apdu_buffer + offset + 32 - xlength, signature + xoffset,
            xlength);
    offset += 32;
    xoffset += xlength + 2;  // move over rvalue and TagLEn
    // copy s value
    xlength = signature[xoffset - 1];
    if (xlength == 33) {
        xlength = 32;
        xoffset++;
    }
    memmove(G_io_apdu_buffer + offset + 32 - xlength, signature + xoffset,
            xlength);
}

uint32_t set_result_get_publicKey() {
    uint32_t tx = 0;
    G_io_apdu_buffer[tx++] = 65;
    memcpy(G_io_apdu_buffer + tx, tmpCtx.publicKeyContext.publicKey.W, 65);
    tx += 65;
    G_io_apdu_buffer[tx++] = 40;
    memcpy(G_io_apdu_buffer + tx, tmpCtx.publicKeyContext.address, 40);
    tx += 40;
    if (tmpCtx.publicKeyContext.getChaincode) {
        memcpy(G_io_apdu_buffer + tx, tmpCtx.publicKeyContext.chainCode, 32);
        tx += 32;
    }
    return tx;
}

volatile uint8_t appState;

void reset_app_context() {
    appState = APP_STATE_IDLE;
    PRINTF("Resetting context\n");
    memset(tmpCtx.transactionContext.tokenSet, 0, MAX_TOKEN);
    memset(&txContext, 0, sizeof(txContext));
    memset(&tmpContent, 0, sizeof(tmpContent));
}

#include "uint256.h"

#define WEI_TO_ETHER 18

tokenDefinition_t* getKnownToken(uint8_t* tokenAddr) {
    tokenDefinition_t* currentToken = NULL;

    for (int i = 0; i < MAX_TOKEN; i++) {
        currentToken = &tmpCtx.transactionContext.tokens[i];
        if (tmpCtx.transactionContext.tokenSet[i] &&
            (memcmp(currentToken->address, tokenAddr, EVM_ADDRESS_LEN) == 0)) {
            PRINTF("Token found at index %d\n", i);
            return currentToken;
        }
    }

    return NULL;
}

static uint32_t splitBinaryParameterPart(char* result, uint8_t* parameter) {
    uint32_t i;
    for (i = 0; i < 8; i++) {
        if (parameter[i] != 0x00) {
            break;
        }
    }
    if (i == 8) {
        result[0] = '0';
        result[1] = '0';
        result[2] = '\0';
        return 2;
    } else {
        array_hexstr(result, parameter + i, 8 - i);
        return ((8 - i) * 2);
    }
}

// Number of bytes an ABI string of `length` bytes occupies, including its zero
// padding.
#define ABI_PADDED_LEN(length) \
    ((((length) + (INT256_LENGTH - 1)) / INT256_LENGTH) * INT256_LENGTH)

// Reads a 32-byte ABI word as a uint32_t. The words read this way are a string
// offset and a string length, both bounded by the APDU transport, so a word
// with anything set outside its low four bytes is malformed rather than merely
// large.
static bool readAbiWord(const uint8_t* word, uint32_t* value) {
    for (size_t i = 0; i < INT256_LENGTH - 4; i++) {
        if (word[i] != 0) {
            return false;
        }
    }
    *value = U4BE(word, INT256_LENGTH - 4);
    return true;
}

// Validates the comment's ABI header and yields its length. Every deviation
// from the canonical encoding is refused rather than accommodated: a payload
// whose layout cannot be determined exactly is a payload that cannot be shown
// to the user.
static bool getTokenCommentLength(const txContext_t* context,
                                  uint32_t* commentLength) {
    uint32_t offset;
    uint32_t length;

    if (!readAbiWord(dataContext.tokenContext.commentHeader, &offset) ||
        (offset != TOKEN_COMMENT_ABI_OFFSET)) {
        PRINTF("Non-canonical comment offset\n");
        return false;
    }
    if (!readAbiWord(dataContext.tokenContext.commentHeader + INT256_LENGTH,
                     &length) ||
        (length > TOKEN_COMMENT_MAX_LEN)) {
        PRINTF("Comment too long to display\n");
        return false;
    }
    // The comment's padded length fixes the length of the whole data field; a
    // mismatch means the field carries bytes the ABI layout does not account
    // for.
    if (context->currentFieldLength !=
        TOKEN_COMMENT_DATA_POS + ABI_PADDED_LEN(length)) {
        PRINTF("Comment length does not match data field length\n");
        return false;
    }
    *commentLength = length;
    return true;
}

// A comment is only clear-signed if the user reads exactly what was signed.
// Control characters and non-ASCII bytes can render as something other than
// themselves, and an embedded NUL would hide every byte after it, so they are
// refused rather than displayed.
static bool isDisplayableComment(const uint8_t* bytes, uint32_t length) {
    for (uint32_t i = 0; i < length; i++) {
        if ((bytes[i] < 0x20) || (bytes[i] > 0x7e)) {
            PRINTF("Comment is not printable ASCII\n");
            return false;
        }
    }
    return true;
}

// Splits an incoming chunk of token transfer calldata across the fixed regions
// of its ABI layout: the transfer prefix, the comment header, the comment
// bytes, then the trailing padding. Each region is copied only into its own
// buffer, and regions this signer does not parse are hashed without being
// stored, so calldata longer than the layout cannot write past any buffer.
static bool copyTokenTransferData(txContext_t* context, uint32_t copySize) {
    while (copySize != 0) {
        uint32_t pos = context->currentFieldPos;
        uint32_t commentLength;
        uint32_t chunk;
        uint8_t* dest;
        bool isComment = false;

        if (pos < TOKEN_TRANSFER_DATA_LEN) {
            chunk = MIN(copySize, TOKEN_TRANSFER_DATA_LEN - pos);
            dest = dataContext.tokenContext.data + pos;
        } else if (pos < TOKEN_COMMENT_DATA_POS) {
            chunk = MIN(copySize, TOKEN_COMMENT_DATA_POS - pos);
            dest = dataContext.tokenContext.commentHeader +
                   (pos - TOKEN_TRANSFER_DATA_LEN);
        } else if (!getTokenCommentLength(context, &commentLength)) {
            return false;
        } else if (pos < TOKEN_COMMENT_DATA_POS + commentLength) {
            chunk = MIN(copySize, TOKEN_COMMENT_DATA_POS + commentLength - pos);
            dest = (uint8_t*)dataContext.tokenContext.comment +
                   (pos - TOKEN_COMMENT_DATA_POS);
            isComment = true;
        } else {
            // Zero padding after the comment: hashed as part of the preimage,
            // but there is nothing in it to show.
            chunk = copySize;
            dest = NULL;
        }

        if (copyTxData(context, dest, chunk) != 0) {
            return false;
        }
        if (isComment && !isDisplayableComment(dest, chunk)) {
            return false;
        }
        copySize -= chunk;
    }
    // A payload that stops at the end of the header declares a comment the
    // field has no room for, so the header is validated here too rather than
    // only when comment bytes are reached.
    if (context->currentFieldPos >= TOKEN_COMMENT_DATA_POS) {
        uint32_t commentLength;
        if (!getTokenCommentLength(context, &commentLength)) {
            return false;
        }
    }
    return true;
}

customStatus_e customProcessor(txContext_t* context) {
    if (((context->txType == CELO_LEGACY &&
          context->currentField == CELO_LEGACY_RLP_DATA) ||
         (context->txType == CIP64 &&
          context->currentField == CIP64_RLP_DATA) ||
         (context->txType == EIP1559 &&
          context->currentField == EIP1559_RLP_DATA)) &&
        (context->currentFieldLength != 0)) {
        dataPresent = true;
        // If handling a new contract rather than a function call, abort
        // immediately
        if (tmpContent.txContent.destinationLength == 0) {
            return CUSTOM_NOT_HANDLED;
        }
        if (context->currentFieldPos == 0) {
            // If handling the beginning of the data field, assume that the
            // function selector is present
            if (context->commandLength < 4) {
                PRINTF("Missing function selector\n");
                return CUSTOM_FAULT;
            }
            // Initial check to see if the token content can be processed. Both
            // methods are accepted only at the exact length their ABI layout
            // implies; transferWithComment additionally carries a comment,
            // whose length is checked against the field length once its header
            // has been read.
            if ((((context->currentFieldLength == TOKEN_TRANSFER_DATA_LEN) &&
                  (memcmp(context->workBuffer, TOKEN_TRANSFER_ID, 4) == 0)) ||
                 ((context->currentFieldLength >= TOKEN_COMMENT_DATA_POS) &&
                  (memcmp(context->workBuffer, TOKEN_TRANSFER_WITH_COMMENT_ID,
                          4) == 0))) &&
                (getKnownToken(tmpContent.txContent.destination) != NULL)) {
                provisionType = PROVISION_TOKEN;
                // The comment is NUL-terminated by this wipe, not by the
                // parser: a transaction with no comment must not inherit the
                // previous one's.
                explicit_bzero(&dataContext.tokenContext,
                               sizeof(dataContext.tokenContext));
            }
            // Initial check to see if the lock content can be processed
            if ((context->currentFieldLength ==
                 sizeof(dataContext.lockContext.data)) &&
                (memcmp(context->workBuffer, LOCK_METHOD_ID, 4) == 0)) {
                provisionType = PROVISION_LOCK;
            }
            // Initial check to see if the vote content can be processed
            if ((context->currentFieldLength ==
                 sizeof(dataContext.voteContext.data)) &&
                (memcmp(context->workBuffer, VOTE_METHOD_ID, 4) == 0)) {
                provisionType = PROVISION_VOTE;
            }
            // Initial check to see if the activate content can be processed
            if ((context->currentFieldLength ==
                 sizeof(dataContext.activateContext.data)) &&
                (memcmp(context->workBuffer, ACTIVATE_METHOD_ID, 4) == 0)) {
                provisionType = PROVISION_ACTIVATE;
            }
            // Initial check to see if the revoke content can be processed
            if ((context->currentFieldLength ==
                 sizeof(dataContext.revokeContext.data)) &&
                ((memcmp(context->workBuffer, REVOKE_PENDING_METHOD_ID, 4) ==
                  0) ||
                 (memcmp(context->workBuffer, REVOKE_ACTIVE_METHOD_ID, 4) ==
                  0))) {
                provisionType = PROVISION_REVOKE;
            }
            // Initial check to see if the unlock content can be processed
            if ((context->currentFieldLength ==
                 sizeof(dataContext.unlockContext.data)) &&
                (memcmp(context->workBuffer, UNLOCK_METHOD_ID, 4) == 0)) {
                provisionType = PROVISION_UNLOCK;
            }
            // Initial check to see if the withdraw content can be processed
            if ((context->currentFieldLength ==
                 sizeof(dataContext.withdrawContext.data)) &&
                (memcmp(context->workBuffer, WITHDRAW_METHOD_ID, 4) == 0)) {
                provisionType = PROVISION_WITHDRAW;
            }
            // Initial check to see if the relock content can be processed
            if ((context->currentFieldLength ==
                 sizeof(dataContext.relockContext.data)) &&
                (memcmp(context->workBuffer, RELOCK_METHOD_ID, 4) == 0)) {
                provisionType = PROVISION_RELOCK;
            }
            // Initial check to see if the create account content can be
            // processed
            if ((context->currentFieldLength ==
                 sizeof(dataContext.createAccountContext.data)) &&
                (memcmp(context->workBuffer, CREATE_ACCOUNT_METHOD_ID, 4) ==
                 0)) {
                provisionType = PROVISION_CREATE_ACCOUNT;
            }
        }
        if (provisionType != PROVISION_NONE) {
            if (context->currentFieldPos < context->currentFieldLength) {
                uint32_t copySize =
                    (context->commandLength < ((context->currentFieldLength -
                                                context->currentFieldPos))
                         ? context->commandLength
                         : context->currentFieldLength -
                               context->currentFieldPos);
                switch (provisionType) {
                    case PROVISION_TOKEN:
                        if (!copyTokenTransferData(context, copySize)) {
                            return CUSTOM_FAULT;
                        }
                        break;
                    case PROVISION_LOCK:
                        copyTxData(context,
                                   dataContext.lockContext.data +
                                       context->currentFieldPos,
                                   copySize);
                        break;
                    case PROVISION_VOTE:
                        copyTxData(context,
                                   dataContext.voteContext.data +
                                       context->currentFieldPos,
                                   copySize);
                        break;
                    case PROVISION_ACTIVATE:
                        copyTxData(context,
                                   dataContext.activateContext.data +
                                       context->currentFieldPos,
                                   copySize);
                        break;
                    case PROVISION_REVOKE:
                        copyTxData(context,
                                   dataContext.revokeContext.data +
                                       context->currentFieldPos,
                                   copySize);
                        break;
                    case PROVISION_UNLOCK:
                        copyTxData(context,
                                   dataContext.unlockContext.data +
                                       context->currentFieldPos,
                                   copySize);
                        break;
                    case PROVISION_WITHDRAW:
                        copyTxData(context,
                                   dataContext.withdrawContext.data +
                                       context->currentFieldPos,
                                   copySize);
                        break;
                    case PROVISION_RELOCK:
                        copyTxData(context,
                                   dataContext.relockContext.data +
                                       context->currentFieldPos,
                                   copySize);
                        break;
                    case PROVISION_CREATE_ACCOUNT:
                        copyTxData(context,
                                   dataContext.createAccountContext.data +
                                       context->currentFieldPos,
                                   copySize);
                        break;
                    default:
                        break;
                }
            }
            if (context->currentFieldPos == context->currentFieldLength) {
                context->currentField++;
                context->processingField = false;
            }
            return CUSTOM_HANDLED;
        } else {
            uint32_t blockSize;
            uint32_t copySize;
            uint32_t fieldPos = context->currentFieldPos;
            if (fieldPos == 0) {
                if (!N_storage.dataAllowed) {
                    PRINTF("Data field forbidden\n");
                    return CUSTOM_BLIND_SIGNING_OFF;
                }
                if (!N_storage.contractDetails) {
                    return CUSTOM_NOT_HANDLED;
                }
                dataContext.rawDataContext.fieldIndex = 0;
                dataContext.rawDataContext.fieldOffset = 0;
                blockSize = 4;
            } else {
                if (!N_storage.contractDetails) {
                    return CUSTOM_NOT_HANDLED;
                }
                blockSize = 32 - (dataContext.rawDataContext.fieldOffset % 32);
            }

            // Sanity check
            if ((context->currentFieldLength - fieldPos) < blockSize) {
                PRINTF("Unconsistent data\n");
                return CUSTOM_FAULT;
            }

            copySize =
                (context->commandLength < blockSize ? context->commandLength
                                                    : blockSize);
            copyTxData(context,
                       dataContext.rawDataContext.data +
                           dataContext.rawDataContext.fieldOffset,
                       copySize);

            if (context->currentFieldPos == context->currentFieldLength) {
                context->currentField++;
                context->processingField = false;
            }

            dataContext.rawDataContext.fieldOffset += copySize;

            if (copySize == blockSize) {
                // Can display
                if (fieldPos != 0) {
                    dataContext.rawDataContext.fieldIndex++;
                }
                dataContext.rawDataContext.fieldOffset = 0;
                if (fieldPos == 0) {
                    array_hexstr(strings.tmp.tmp,
                                 dataContext.rawDataContext.data, 4);
                    ui_confirm_selector_flow();
                } else {
                    uint32_t offset = 0;
                    uint32_t i;
                    snprintf(strings.tmp.tmp2, sizeof(strings.tmp.tmp2),
                             "Field %d", dataContext.rawDataContext.fieldIndex);
                    for (i = 0; i < 4; i++) {
                        offset += splitBinaryParameterPart(
                            strings.tmp.tmp + offset,
                            dataContext.rawDataContext.data + 8 * i);
                        if (i != 3) {
                            strings.tmp.tmp[offset++] = ':';
                        }
                    }
                    ui_confirm_parameter_flow();
                }
            } else {
                return CUSTOM_HANDLED;
            }

            return CUSTOM_SUSPENDED;
        }
    }
    return CUSTOM_NOT_HANDLED;
}

// Refuses the transaction being parsed. `direct` selects how the caller was
// reached: from the APDU handler, which expects an exception, or from a
// deferred UI callback, which expects the status on the wire. Callers must
// return immediately afterwards.
static void refuseTransaction(bool direct) {
    reset_app_context();
    if (direct) {
        THROW(SW_ERROR_IN_DATA);
    }
    io_seproxyhal_send_status(SW_ERROR_IN_DATA);
    ui_idle();
}

// Renders "<amount> <ticker>" into a review buffer. Tickers that carry their
// own leading space get no separator. Refuses rather than truncating: a clipped
// amount or ticker would misrepresent what is about to be signed.
static bool formatAmountWithTicker(char* out, size_t outSize,
                                   const char* amount, const char* ticker) {
    const char* separator =
        ((ticker[0] != '\0') && (ticker[0] != ' ')) ? " " : "";
    int written = snprintf(out, outSize, "%s%s%s", amount, separator, ticker);
    return (written >= 0) && ((size_t)written < outSize);
}

void finalizeParsing(bool direct) {
    uint256_t gasPrice, startGas, uint256;
    uint32_t i;
    uint8_t decimals = WEI_TO_ETHER;
    uint8_t feeDecimals = WEI_TO_ETHER;
    const char* ticker = " " CHAINID_COINNAME;
    const char* feeTicker = " " CHAINID_COINNAME;

    // Display correct currency if fee currency field sent
    if (tmpContent.txContent.feeCurrencyLength != 0) {
        tokenDefinition_t* feeCurrencyToken =
            getKnownToken(tmpContent.txContent.feeCurrency);
        if (feeCurrencyToken == NULL) {
            PRINTF("Invalid fee currency\n");
            refuseTransaction(direct);
            return;
        }
        feeTicker = feeCurrencyToken->ticker;
        feeDecimals = feeCurrencyToken->decimals;
    }

    // Store the hash
    CX_THROW(cx_hash_no_throw((cx_hash_t*)&sha3, CX_LAST,
                              tmpCtx.transactionContext.hash, 0,
                              tmpCtx.transactionContext.hash, 32));
    // If there is a token to process, check if it is well known
    if (provisionType == PROVISION_TOKEN) {
        tokenDefinition_t* currentToken =
            getKnownToken(tmpContent.txContent.destination);
        if (currentToken == NULL) {
            // The metadata that made this calldata clear-signable was evicted
            // after it was classified, so the amount and recipient below cannot
            // be labelled. Refusing is the only safe outcome: the calldata has
            // already bypassed the blind-signing gate by virtue of being a
            // recognised token transfer.
            PRINTF("Token metadata no longer known\n");
            refuseTransaction(direct);
            return;
        }
        dataPresent = false;
        decimals = currentToken->decimals;
        ticker = currentToken->ticker;
        tmpContent.txContent.destinationLength = EVM_ADDRESS_LEN;
        memcpy(tmpContent.txContent.destination,
               dataContext.tokenContext.data + 4 + 12, EVM_ADDRESS_LEN);
        memcpy(tmpContent.txContent.value.value,
               dataContext.tokenContext.data + 4 + 32, 32);
        tmpContent.txContent.value.length = 32;
    } else if (provisionType == PROVISION_VOTE) {
        tmpContent.txContent.destinationLength = 20;
        memcpy(tmpContent.txContent.destination,
               dataContext.voteContext.data + 4 + 12, 20);
        memcpy(tmpContent.txContent.value.value,
               dataContext.voteContext.data + 4 + 32, 32);
        tmpContent.txContent.value.length = 32;
    } else if (provisionType == PROVISION_ACTIVATE) {
        tmpContent.txContent.destinationLength = 20;
        memcpy(tmpContent.txContent.destination,
               dataContext.activateContext.data + 4 + 12, 20);
    } else if (provisionType == PROVISION_REVOKE) {
        tmpContent.txContent.destinationLength = 20;
        memcpy(tmpContent.txContent.destination,
               dataContext.revokeContext.data + 4 + 12, 20);
        memcpy(tmpContent.txContent.value.value,
               dataContext.revokeContext.data + 4 + 32, 32);
        tmpContent.txContent.value.length = 32;
    } else if (provisionType == PROVISION_UNLOCK) {
        memcpy(tmpContent.txContent.value.value,
               dataContext.unlockContext.data + 4, 32);
        tmpContent.txContent.value.length = 32;
    } else if (provisionType == PROVISION_RELOCK) {
        memcpy(tmpContent.txContent.value.value,
               dataContext.relockContext.data + 4 + 32, 32);
        tmpContent.txContent.value.length = 32;
    } else if (provisionType == PROVISION_WITHDRAW) {
        memcpy(tmpContent.txContent.value.value,
               dataContext.withdrawContext.data + 4, 32);
        tmpContent.txContent.value.length = 32;
    } else if (provisionType == PROVISION_CREATE_ACCOUNT) {
        // no parameters to extract; selector only
    } else {
        if (dataPresent && !N_storage.dataAllowed &&
            provisionType != PROVISION_LOCK) {
            reset_app_context();
            PRINTF("Data field forbidden\n");
            if (direct) {
                THROW(SW_ERROR_IN_DATA);
            } else {
                io_seproxyhal_send_status(SW_ERROR_IN_DATA);
                ui_error_blind_signing();
                return;
            }
        }
    }
    // Add address
    if (tmpContent.txContent.destinationLength != 0) {
        char address[41];
        getEthAddressStringFromBinary(tmpContent.txContent.destination, address,
                                      CHAIN_ID, &sha3);
        strings.common.fullAddress[0] = '0';
        strings.common.fullAddress[1] = 'x';
        memcpy(strings.common.fullAddress + 2, address, 40);
        strings.common.fullAddress[42] = '\0';
    } else {
        strcpy(strings.common.fullAddress, "New Contract");
    }
    // Add gateway fee recipient address
    if (tmpContent.txContent.gatewayDestinationLength != 0) {
        char gatewayAddress[41];
        getEthAddressStringFromBinary(tmpContent.txContent.gatewayDestination,
                                      gatewayAddress, CHAIN_ID, &sha3);
        strings.common.fullGatewayAddress[0] = '0';
        strings.common.fullGatewayAddress[1] = 'x';
        memcpy(strings.common.fullGatewayAddress + 2, gatewayAddress, 40);
        strings.common.fullGatewayAddress[42] = '\0';
    }
    // Add amount in ethers or tokens
    convertUint256BE(tmpContent.txContent.value.value,
                     tmpContent.txContent.value.length, &uint256);
    tostring256(&uint256, 10, (char*)(G_io_apdu_buffer + 100), 100);
    i = 0;
    while (G_io_apdu_buffer[100 + i]) {
        i++;
    }

    if (!adjustDecimals((char*)(G_io_apdu_buffer + 100), i,
                        (char*)G_io_apdu_buffer, 100, decimals)) {
        PRINTF("Cannot format amount\n");
        refuseTransaction(direct);
        return;
    }

    if (!formatAmountWithTicker(strings.common.fullAmount,
                                sizeof(strings.common.fullAmount),
                                (char*)G_io_apdu_buffer, ticker)) {
        PRINTF("Amount does not fit the review buffer\n");
        refuseTransaction(direct);
        return;
    }

    // Add gateway fee
    convertUint256BE(tmpContent.txContent.gatewayFee.value,
                     tmpContent.txContent.gatewayFee.length, &uint256);
    tostring256(&uint256, 10, (char*)(G_io_apdu_buffer + 100), 100);
    i = 0;
    while (G_io_apdu_buffer[100 + i]) {
        i++;
    }

    if (!adjustDecimals((char*)(G_io_apdu_buffer + 100), i,
                        (char*)G_io_apdu_buffer, 100, feeDecimals)) {
        PRINTF("Cannot format gateway fee\n");
        refuseTransaction(direct);
        return;
    }

    if (!formatAmountWithTicker(strings.common.gatewayFee,
                                sizeof(strings.common.gatewayFee),
                                (char*)G_io_apdu_buffer, feeTicker)) {
        PRINTF("Gateway fee does not fit the review buffer\n");
        refuseTransaction(direct);
        return;
    }

    // Compute maximum fee
    convertUint256BE(tmpContent.txContent.gasprice.value,
                     tmpContent.txContent.gasprice.length, &gasPrice);
    convertUint256BE(tmpContent.txContent.startgas.value,
                     tmpContent.txContent.startgas.length, &startGas);
    mul256(&gasPrice, &startGas, &uint256);
    tostring256(&uint256, 10, (char*)(G_io_apdu_buffer + 100), 100);
    i = 0;
    while (G_io_apdu_buffer[100 + i]) {
        i++;
    }

    if (!adjustDecimals((char*)(G_io_apdu_buffer + 100), i,
                        (char*)G_io_apdu_buffer, 100, feeDecimals)) {
        PRINTF("Cannot format max fee\n");
        refuseTransaction(direct);
        return;
    }

    if (!formatAmountWithTicker(strings.common.maxFee,
                                sizeof(strings.common.maxFee),
                                (char*)G_io_apdu_buffer, feeTicker)) {
        PRINTF("Max fee does not fit the review buffer\n");
        refuseTransaction(direct);
        return;
    }

    switch (provisionType) {
        case PROVISION_LOCK:
            strcpy(strings.common.stakingType, "Lock");
            break;
        case PROVISION_VOTE:
            strcpy(strings.common.stakingType, "Vote");
            break;
        case PROVISION_ACTIVATE:
            strcpy(strings.common.stakingType, "Activate");
            break;
        case PROVISION_REVOKE:
            strcpy(strings.common.stakingType, "Revoke");
            break;
        case PROVISION_UNLOCK:
            strcpy(strings.common.stakingType, "Unlock");
            break;
        case PROVISION_WITHDRAW:
            strcpy(strings.common.stakingType, "Withdraw");
            break;
        case PROVISION_RELOCK:
            strcpy(strings.common.stakingType, "Relock");
            break;
        case PROVISION_CREATE_ACCOUNT:
            strcpy(strings.common.stakingType, "Create Account");
            break;
        default:
            break;
    }

#ifdef HAVE_SWAP
    // If we are in swap context, do not redisplay the message data
    // Instead, ensure they are identical with what was previously displayed
    if (G_called_from_swap) {
        if (G_swap_response_ready) {
            // Safety against trying to make the app sign multiple TX
            // This code should never be triggered as the app is supposed to
            // exit after sending the signed transaction
            PRINTF("Safety against double signing triggered\n");
            reset_app_context();
            io_seproxyhal_send_status(SW_SWAP_CHECKING_FAIL);
            os_sched_exit(-1);
        } else {
            // We will quit the app after this transaction, whether it succeeds
            // or fails
            PRINTF(
                "Swap response is ready, the app will quit after the next "
                "send\n");
            // This boolean will make the io_send_sw family instant reply +
            // return to exchange
            G_swap_response_ready = true;
        }
        if (swap_check_validity()) {
            PRINTF("Swap response validated\n");
            io_sign_and_send_tx();
            swap_finalize_exchange_sign_transaction(true);
        } else {
            PRINTF("swap_check_validity failed\n");
            // Failsafe
            io_seproxyhal_send_status(SW_SWAP_CHECKING_FAIL);
            swap_finalize_exchange_sign_transaction(false);
        }
    } else {
        ui_display_transaction();
    }
#else
    ui_display_transaction();
#endif
}

void ui_display_transaction() {
    switch (provisionType) {
        case PROVISION_LOCK:
        case PROVISION_UNLOCK:
            ui_approval_celo_lock_unlock_flow();
            break;
        case PROVISION_WITHDRAW:
            ui_approval_celo_withdraw_flow();
            break;
        case PROVISION_VOTE:
        case PROVISION_REVOKE:
            ui_approval_celo_vote_revoke_flow();
            break;
        case PROVISION_ACTIVATE:
            ui_approval_celo_activate_flow();
            break;
        case PROVISION_RELOCK:
            ui_approval_celo_relock_flow();
            break;
        case PROVISION_CREATE_ACCOUNT:
            ui_approval_celo_create_account_flow();
            break;
        default: {
            // Contract creation never reaches the per-field review, so the
            // contractDetails setting is not evidence that its init code was
            // seen: warn regardless of it.
            bool isContractCreation =
                (tmpContent.txContent.destinationLength == 0);
            bool needsDataWarning = dataPresent && (isContractCreation ||
                                                    !N_storage.contractDetails);
            // dataContext is a union, so the comment only exists for a token
            // transfer.
            bool hasComment = (provisionType == PROVISION_TOKEN) &&
                              (dataContext.tokenContext.comment[0] != '\0');

            if (tmpContent.txContent.gatewayDestinationLength != 0) {
                if (needsDataWarning) {
                    ui_approval_celo_data_warning_gateway_tx_flow();
                } else if (hasComment) {
                    ui_approval_celo_gateway_tx_comment_flow();
                } else {
                    ui_approval_celo_gateway_tx_flow();
                }
            } else {
                if (needsDataWarning) {
                    ui_approval_celo_data_warning_tx_flow();
                } else if (hasComment) {
                    ui_approval_celo_tx_comment_flow();
                } else {
                    ui_approval_celo_tx_flow();
                }
            }
        }
    }
}
