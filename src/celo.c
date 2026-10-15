#include "celo.h"

#include <string.h>

#include "ethUtils.h"
#include "globals.h"
#include "handle_copy_transaction_parameters.h"
#include "os.h"
#include "swap_utils.h"
#include "ui_common.h"
#include "utils.h"

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

// Celo core contracts, per network. A recognised staking selector makes
// finalizeParsing() replace the displayed destination with an address taken
// from the calldata and relabel the review as an official operation, so the
// decode must be bound to the contracts that actually implement these methods.
// The addresses are the Registry-resolved proxies, which implementation
// upgrades do not move; a governance repoint would cost clear-signing, not the
// ability to transact. Alfajores and Baklava are absent because both networks
// were retired and their addresses are no longer resolvable.
typedef enum {
    CELO_LOCKED_GOLD,
    CELO_ELECTION,
    CELO_ACCOUNTS,
    CELO_CONTRACT_COUNT
} celo_contract_t;

typedef struct celoNetwork_t {
    uint32_t chainId;
    uint8_t contracts[CELO_CONTRACT_COUNT][EVM_ADDRESS_LEN];
} celoNetwork_t;

static const celoNetwork_t CELO_NETWORKS[] = {
    {42220,  // Mainnet
     {       // LockedGold
      {0x6c, 0xc0, 0x83, 0xae, 0xd9, 0xe3, 0xeb, 0xe3, 0x02, 0xa6,
       0x33, 0x6d, 0xbc, 0x7c, 0x92, 0x1c, 0x9f, 0x03, 0x34, 0x9e},
      // Election
      {0x8d, 0x66, 0x77, 0x19, 0x21, 0x44, 0x29, 0x28, 0x70, 0x90,
       0x7e, 0x3f, 0xa8, 0xa5, 0x52, 0x7f, 0xe5, 0x5a, 0x7f, 0xf6},
      // Accounts
      {0x7d, 0x21, 0x68, 0x5c, 0x17, 0x60, 0x73, 0x38, 0xb3, 0x13,
       0xa7, 0x17, 0x4b, 0xab, 0x66, 0x20, 0xba, 0xd0, 0xaa, 0xb7}}},
    {11142220,  // Celo Sepolia
     {          // LockedGold
      {0x3d, 0xb0, 0xf0, 0x85, 0x0c, 0x5b, 0x5f, 0x42, 0xfe, 0x30,
       0xd6, 0x87, 0x78, 0xc8, 0x95, 0x8f, 0xc5, 0xee, 0x79, 0x51},
      // Election
      {0xeb, 0x8b, 0x62, 0x6f, 0x3a, 0x76, 0x17, 0x4f, 0x45, 0x76,
       0xbb, 0x47, 0x42, 0x9c, 0x47, 0xef, 0xde, 0xd7, 0xc2, 0x11},
      // Accounts
      {0x44, 0x95, 0x72, 0x32, 0x69, 0x9c, 0xa0, 0x60, 0xb6, 0x07,
       0xe7, 0x70, 0x83, 0xbd, 0xac, 0xd3, 0x50, 0xd6, 0xb6, 0xd1}}},
};

// Each staking call this app clear-signs, bound to the contract implementing it
// and to the exact calldata length its ABI implies. The selectors alone are not
// enough: withdraw(uint256) in particular is one of the most widely deployed
// signatures in DeFi, so without the contract binding an honest unwrap on an
// unrelated contract would be presented as a Celo withdrawal.
typedef struct stakingMethod_t {
    // Held inline rather than as a pointer: a pointer stored in a const
    // structure is a link-time address, and BOLOS relocates the app.
    uint8_t selector[SELECTOR_LEN];
    celo_contract_t contract;
    uint32_t dataLength;
    provision_type_t provision;
} stakingMethod_t;

static const stakingMethod_t STAKING_METHODS[] = {
    {{0xf8, 0x3d, 0x08, 0xba},  // lock
     CELO_LOCKED_GOLD,
     sizeof(dataContext.lockContext.data),
     PROVISION_LOCK},
    {{0x61, 0x98, 0xe3, 0x39},  // unlock
     CELO_LOCKED_GOLD,
     sizeof(dataContext.unlockContext.data),
     PROVISION_UNLOCK},
    {{0xb2, 0xfb, 0x30, 0xcb},  // relock
     CELO_LOCKED_GOLD,
     sizeof(dataContext.relockContext.data),
     PROVISION_RELOCK},
    {{0x2e, 0x1a, 0x7d, 0x4d},  // withdraw
     CELO_LOCKED_GOLD,
     sizeof(dataContext.withdrawContext.data),
     PROVISION_WITHDRAW},
    {{0x58, 0x0d, 0x74, 0x7a},  // vote
     CELO_ELECTION,
     sizeof(dataContext.voteContext.data),
     PROVISION_VOTE},
    {{0x1c, 0x5a, 0x9d, 0x9c},  // activate
     CELO_ELECTION,
     sizeof(dataContext.activateContext.data),
     PROVISION_ACTIVATE},
    {{0x9d, 0xfb, 0x60, 0x81},  // revoke_pending
     CELO_ELECTION,
     sizeof(dataContext.revokeContext.data),
     PROVISION_REVOKE},
    {{0x6e, 0x19, 0x84, 0x75},  // revoke_active
     CELO_ELECTION,
     sizeof(dataContext.revokeContext.data),
     PROVISION_REVOKE},
    {{0x9d, 0xca, 0x36, 0x2f},  // create_account
     CELO_ACCOUNTS,
     sizeof(dataContext.createAccountContext.data),
     PROVISION_CREATE_ACCOUNT},
};

// The chain ID was parsed before the data field, so the network is known by the
// time the calldata is classified.
static const celoNetwork_t* currentCeloNetwork(void) {
    uint32_t chainId = 0;
    if ((tmpContent.txContent.vLength == 0) ||
        (tmpContent.txContent.vLength > sizeof(chainId))) {
        return NULL;
    }
    for (uint8_t i = 0; i < tmpContent.txContent.vLength; i++) {
        chainId = (chainId << 8) | tmpContent.txContent.v[i];
    }
    for (size_t i = 0; i < sizeof(CELO_NETWORKS) / sizeof(CELO_NETWORKS[0]);
         i++) {
        if (CELO_NETWORKS[i].chainId == chainId) {
            return &CELO_NETWORKS[i];
        }
    }
    return NULL;
}

// Recognises a staking call only when the selector, the calldata length and the
// destination contract on this network all agree.
static provision_type_t classifyStakingCall(const txContext_t* context) {
    const celoNetwork_t* network = currentCeloNetwork();

    if ((network == NULL) ||
        (tmpContent.txContent.destinationLength != EVM_ADDRESS_LEN)) {
        return PROVISION_NONE;
    }
    for (size_t i = 0; i < sizeof(STAKING_METHODS) / sizeof(STAKING_METHODS[0]);
         i++) {
        const stakingMethod_t* method = &STAKING_METHODS[i];
        if ((context->currentFieldLength == method->dataLength) &&
            (memcmp(context->workBuffer, method->selector, SELECTOR_LEN) ==
             0) &&
            (memcmp(tmpContent.txContent.destination,
                    network->contracts[method->contract],
                    EVM_ADDRESS_LEN) == 0)) {
            return method->provision;
        }
    }
    return PROVISION_NONE;
}

// Only lock() is payable on the core contracts, and the amount it locks is the
// transaction's own value, which its review shows. The review of every other
// clear-signed call shows an amount from the calldata or none, never the
// transaction's own value, so such a call must not carry one.
static bool provisionAcceptsNativeValue(provision_type_t provision) {
    return provision == PROVISION_LOCK;
}

// RLP integers carry no leading zeros but the parser does not insist on it, so
// the bytes are judged and not the length.
static bool nativeValueIsZero(void) {
    for (uint8_t i = 0; i < tmpContent.txContent.value.length; i++) {
        if (tmpContent.txContent.value.value[i] != 0) {
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
            // The staking decode hides the real contract behind a
            // friendly label, so it is applied only to the Celo core contracts.
            provision_type_t staking = classifyStakingCall(context);
            if (staking != PROVISION_NONE) {
                provisionType = staking;
            }
            // The value field precedes the data, so it is complete here.
            if ((provisionType != PROVISION_NONE) &&
                !provisionAcceptsNativeValue(provisionType) &&
                !nativeValueIsZero()) {
                PRINTF("Native value forbidden for this call\n");
                return CUSTOM_FAULT;
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

// finalizeParsing() formats every amount through G_io_apdu_buffer:
// tostring256() writes the decimal digits into the upper half and
// adjustDecimals() scales them into the lower half. The two halves are the same
// size, so one constant names both the bound and the offset.
// tests/unit/test_eth_utils.c pins which decimals this bound admits.
#define AMOUNT_SCRATCH_LEN 100

// Renders a value as "<amount> <ticker>" into a review buffer. Refuses at every
// step rather than displaying a value that misrepresents what is being signed.
static bool formatAmountForReview(char* out, size_t outSize,
                                  const uint256_t* value, uint8_t decimals,
                                  const char* ticker) {
    char* digits = (char*)(G_io_apdu_buffer + AMOUNT_SCRATCH_LEN);
    uint32_t digitCount = 0;

    if (!tostring256(value, 10, digits, AMOUNT_SCRATCH_LEN)) {
        return false;
    }
    while (digits[digitCount] != '\0') {
        digitCount++;
    }
    if (!adjustDecimals(digits, digitCount, (char*)G_io_apdu_buffer,
                        AMOUNT_SCRATCH_LEN, decimals)) {
        return false;
    }
    return formatAmountWithTicker(out, outSize, (char*)G_io_apdu_buffer,
                                  ticker);
}

void finalizeParsing(bool direct) {
    uint256_t gasPrice, startGas, uint256;
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

    // The value is replaced by the calldata amount, or left off the screen,
    // further down: where that happens it is checked again, so the review can
    // never stand in for a value it does not show.
    if ((provisionType != PROVISION_NONE) &&
        !provisionAcceptsNativeValue(provisionType) && !nativeValueIsZero()) {
        PRINTF("Native value forbidden for this call\n");
        refuseTransaction(direct);
        return;
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
    if (!formatAmountForReview(strings.common.fullAmount,
                               sizeof(strings.common.fullAmount), &uint256,
                               decimals, ticker)) {
        PRINTF("Cannot display amount\n");
        refuseTransaction(direct);
        return;
    }

    // Add gateway fee
    convertUint256BE(tmpContent.txContent.gatewayFee.value,
                     tmpContent.txContent.gatewayFee.length, &uint256);
    if (!formatAmountForReview(strings.common.gatewayFee,
                               sizeof(strings.common.gatewayFee), &uint256,
                               feeDecimals, feeTicker)) {
        PRINTF("Cannot display gateway fee\n");
        refuseTransaction(direct);
        return;
    }

    // Compute maximum fee
    convertUint256BE(tmpContent.txContent.gasprice.value,
                     tmpContent.txContent.gasprice.length, &gasPrice);
    convertUint256BE(tmpContent.txContent.startgas.value,
                     tmpContent.txContent.startgas.length, &startGas);
    mul256(&gasPrice, &startGas, &uint256);
    if (!formatAmountForReview(strings.common.maxFee,
                               sizeof(strings.common.maxFee), &uint256,
                               feeDecimals, feeTicker)) {
        PRINTF("Cannot display max fee\n");
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
        if (swap_check_validity(ticker, decimals)) {
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
