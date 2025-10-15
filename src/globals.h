#pragma once

#include "ethUstream.h"
#include "tokens.h"
#include "cx.h"

#define CHAINID_UPCASE "CELO"
#define CHAINID_COINNAME "CELO"
#define CHAIN_ID 0

// RETURN CODES
#define SW_TX_TYPE_NOT_SUPPORTED 0x6501
#define SW_NO_APDU_RECEIVED 0x6982
#define SW_INITIALIZATION_ERROR 0x6985
#define SW_ERROR_IN_DATA 0x6A80
#define SW_ERROR_WRONG_ADDRESS 0x6A83
#define SW_WRONG_P1_OR_P2 0x6b00
#define SW_OK 0x9000
#define SW_INS_NOT_SUPPORTED 0x6d00
#define SW_CLA_NOT_SUPPORTED 0x6e00
#define SW_SWAP_CHECKING_FAIL 0xB00A


typedef union {
  txContent_t txContent;
  cx_sha256_t sha2;
} tmpContent_t;

extern tmpContent_t tmpContent;

extern txContext_t txContext;

#define MAX_TICKER_LEN 16

#define CELO_PRECISION 18

#define MAX_AMOUNT_STR_LEN 21  // 19 for u64 + 1 for '\0' +1 for '.'

#define ADDRESS_LENGTH  41

// The length of a 256-bit integer in bytes.
#define INT256_LENGTH 32

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
    char stakingType[20];
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

extern volatile provision_type_t provisionType;

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

extern dataContext_t dataContext;
