#pragma once

#include "ethUstream.h"
#include "tokens.h"
#include "cx.h"

#define CHAINID_UPCASE "CELO"
#define CHAINID_COINNAME "cGLD"
#define CHAIN_ID 0

typedef union {
  txContent_t txContent;
  cx_sha256_t sha2;
} tmpContent_t;

extern tmpContent_t tmpContent;

extern txContext_t txContext;


#define MAX_TOKEN 2

#define MAX_BIP32_PATH 10

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

typedef struct transactionContext_t {
    bip32Path_t derivationPath;
    uint8_t hash[32];
    tokenDefinition_t tokens[MAX_TOKEN];
    uint8_t tokenSet[MAX_TOKEN];
    uint8_t currentTokenIndex;
} transactionContext_t;

typedef union {
    publicKeyContext_t publicKeyContext;
    transactionContext_t transactionContext;
    messageSigningContext_t messageSigningContext;
} tmpCtx_t;

extern tmpCtx_t tmpCtx;

typedef struct strData_t {
    char fullAddress[43];
    char fullGatewayAddress[43];
    char fullAmount[50];
    char maxFee[50];
    char gatewayFee[50];
} strData_t;

typedef struct strDataTmp_t {
    char tmp[100];
    char tmp2[40];
} strDataTmp_t;

typedef union {
    strData_t common;
    strDataTmp_t tmp;
} strings_t;

extern strings_t strings;

extern volatile bool dataPresent;
extern volatile uint8_t dataAllowed;
extern volatile uint8_t contractDetails;

typedef struct internalStorage_t {
  unsigned char dataAllowed;
  unsigned char contractDetails;
  uint8_t initialized;
} internalStorage_t;

extern const internalStorage_t N_storage_real;
#define N_storage (*(internalStorage_t*) PIC(&N_storage_real))

extern char addressSummary[32];
extern cx_sha3_t sha3;

extern volatile bool tokenProvisioned;

typedef struct tokenContext_t {
    uint8_t data[4 + 32 + 32];
} tokenContext_t;

typedef struct rawDataContext_t {
    uint8_t data[32];
    uint8_t fieldIndex;
    uint8_t fieldOffset;
} rawDataContext_t;

typedef union {
    tokenContext_t tokenContext;
    rawDataContext_t rawDataContext;
} dataContext_t;

extern dataContext_t dataContext;
