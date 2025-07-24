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
#include "constants.h"
#include "ethUstream.h"

// Shared context field sizes
#ifdef SCREEN_SIZE_WALLET
#define SHARED_CTX_FIELD_1_SIZE 380
#else
#define SHARED_CTX_FIELD_1_SIZE 256
#endif
#define SHARED_CTX_FIELD_2_SIZE 40

// --8<-- [start:asset_info]
// NFT

typedef struct nftInfo_t {
    uint8_t contractAddress[ADDRESS_LENGTH];  // must be first item
    char collectionName[COLLECTION_NAME_MAX_LEN + 1];
} nftInfo_t;

// TOKENS

typedef struct tokenDefinition_t {
    uint8_t address[ADDRESS_LENGTH];  // must be first item
    char ticker[MAX_TICKER_LEN];
    uint8_t decimals;
} tokenDefinition_t;

// UNION

typedef union extraInfo_t {
    tokenDefinition_t token;
// Would have used HAVE_NFT_SUPPORT but it is only declared for the Ethereum app
// and not plugins
#ifndef TARGET_NANOS
    nftInfo_t nft;
#endif
} extraInfo_t;
// --8<-- [end:asset_info]

typedef struct tokenContext_t {
    uint8_t data[4 + 32 + 32];
} tokenContext_t;

typedef struct lockContext_t {
    uint8_t data[4];
} lockContext_t;

typedef struct voteContext_t {
    uint8_t data[4 + 32 + 32 + 32 + 32];
} voteContext_t;

typedef struct activateContext_t {
    uint8_t data[4 + 32];
} activateContext_t;

typedef struct revokeContext_t {
    uint8_t data[4 + 32 + 32 + 32 + 32 + 32];
} revokeContext_t;

typedef struct unlockContext_t {
    uint8_t data[4 + 32];
} unlockContext_t;

typedef struct withdrawContext_t {
    uint8_t data[4 + 32];
} withdrawContext_t;

typedef struct relockContext_t {
    uint8_t data[4 + 32 + 32];
} relockContext_t;

typedef struct createAccountContext_t {
    uint8_t data[4];
} createAccountContext_t;

typedef struct rawDataContext_t {
    uint8_t data[32];
    uint8_t fieldIndex;
    uint8_t fieldOffset;
} rawDataContext_t;

typedef union {
    tokenContext_t tokenContext;
    lockContext_t lockContext;
    voteContext_t voteContext;
    activateContext_t activateContext;
    revokeContext_t revokeContext;
    unlockContext_t unlockContext;
    withdrawContext_t withdrawContext;
    relockContext_t relockContext;
    createAccountContext_t createAccountContext;
    rawDataContext_t rawDataContext;
} dataContext_t;

typedef union {
    txContent_t txContent;
    cx_sha256_t sha2;
} tmpContent_t;

typedef struct bip32Path_t {
    uint8_t len;
    uint32_t path[MAX_BIP32_PATH];
} bip32Path_t;

typedef struct publicKeyContext_t {
    cx_ecfp_public_key_t publicKey;
    char address[41];
    uint8_t chainCode[32];
    bool getChaincode;
} publicKeyContext_t;

typedef struct messageSigningContext_t {
    bip32Path_t derivationPath;
    uint8_t hash[32];
    uint32_t remainingLength;
} messageSigningContext_t;
typedef struct messageSigningContext712_t {
    bip32Path_t derivationPath;
    uint8_t domainHash[32];
    uint8_t messageHash[32];
} messageSigningContext712_t;

typedef struct transactionContext_t {
    bip32Path_t derivationPath;
    uint8_t hash[32];
    union extraInfo_t extraInfo[MAX_ASSETS];
    // tokenDefinition_t tokens[MAX_ASSETS]; // km_todo: remove this
    uint8_t assetSet[MAX_ASSETS];
    uint8_t currentAssetIndex;
} transactionContext_t;

typedef union {
    publicKeyContext_t publicKeyContext;
    transactionContext_t transactionContext;
    messageSigningContext_t messageSigningContext;
    messageSigningContext712_t messageSigningContext712;
} tmpCtx_t;

typedef struct strData_t {
    char fullAddress[43];
    char fullGatewayAddress[43];
    char fullAmount[50];
    char maxFee[50];
    char gatewayFee[50];
    char stakingType[20];
} strData_t;

typedef struct strDataTmp_t {
    char tmp[SHARED_CTX_FIELD_1_SIZE];
    char tmp2[SHARED_CTX_FIELD_2_SIZE];
} strDataTmp_t;

typedef union {
    strData_t common;
    strDataTmp_t tmp;
} strings_t;

typedef struct internalStorage_t {
    unsigned char dataAllowed;
    unsigned char contractDetails;
    unsigned char verbose_eip712;
    unsigned char blind_signing;
    uint8_t initialized;
} internalStorage_t;

typedef enum {
    PROVISION_NONE,
    PROVISION_TOKEN,
    PROVISION_LOCK,
    PROVISION_VOTE,
    PROVISION_ACTIVATE,
    PROVISION_REVOKE,
    PROVISION_UNLOCK,
    PROVISION_WITHDRAW,
    PROVISION_RELOCK,
    PROVISION_CREATE_ACCOUNT
} provision_type_t;

// TODO: this should not be exposed
/**
 * @brief Enumeration representing the application state.
 */
typedef enum {
    APP_STATE_IDLE,
    APP_STATE_SIGNING_TX,
    APP_STATE_SIGNING_MESSAGE,
    APP_STATE_SIGNING_EIP712
} app_state_t;

typedef struct chain_config_s {
    char coinName[MAX_TICKER_LEN];  // ticker
    uint64_t chainId;
} chain_config_t;