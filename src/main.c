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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "os.h"
#include "cx.h"
#include "ethUstream.h"
#include "ethUtils.h"
#include "uint256.h"
#include "tokens.h"
#include "celo.h"

#include "os_io_seproxyhal.h"

#include "globals.h"
#include "utils.h"
#include "ui_common.h"
#include "swap_utils.h"

// App flags
#define APP_FLAG_DATA_ALLOWED 0x01
#define APP_FLAG_EXTERNAL_TOKEN_NEEDED 0x02

// App type
#define APP_TYPE 0x02

// APDU instructions
#define CLA 0xE0
#define INS_GET_PUBLIC_KEY 0x02
#define INS_SIGN 0x04
#define INS_GET_APP_CONFIGURATION 0x06
#define INS_SIGN_PERSONAL_MESSAGE 0x08
#define INS_PROVIDE_ERC20_TOKEN_INFORMATION 0x0A
#define INS_GET_APP_TYPE 0x0C
#define P1_CONFIRM 0x01
#define P1_NON_CONFIRM 0x00
#define P2_NO_CHAINCODE 0x00
#define P2_CHAINCODE 0x01
#define P1_FIRST 0x00
#define P1_MORE 0x80

// Common instructions
#define COMMON_CLA 0xB0
#define COMMON_INS_GET_WALLET_ID 0x04

// APDU offsets
#define OFFSET_CLA 0
#define OFFSET_INS 1
#define OFFSET_P1 2
#define OFFSET_P2 3
#define OFFSET_LC 4
#define OFFSET_CDATA 5

// Token signature public key
static const uint8_t CLABS_SIGNATURE_PUBLIC_KEY[] = {
    // cLabs production key (created: May 30, 2024, 17:18:49 GMT+2)
    // akeyless URL: https://ui.gateway.akeyless.celo-networks-dev.org/items?id=281699245&name=%2Fstatic-secrets%2Fdev-tooling-circle%2Fledger-key
    // akeyless path: /static-secrets/dev-tooling-circle
    // Running `openssl ec -in key.pem -text -noout`
    // should output the same hex pub key
    0x04,

    0x59,0xd5,0x59,0xee,0x21,0xa7,0x76,0xfe,
    0x43,0xe4,0xba,0xac,0x14,0x18,0x1c,0x1e,
    0x5f,0xb5,0x1b,0x5b,0x22,0xbc,0x1,0xdd,
    0x46,0xe0,0x60,0xe,0x8d,0xc,0xd7,0xc8,

    0x6d,0xc,0xf2,0x3e,0xe0,0xa3,0x8d,0xc8,
    0x3d,0xa6,0x4b,0xd0,0x2a,0x6d,0x43,0x2c,
    0x86,0x10,0x4a,0x47,0xaf,0xef,0x83,0x83,
    0xc2,0x2b,0xe3,0xd4,0xd1,0xa5,0x32,0x2d
};

static const uint8_t LEDGER_SIGNATURE_PUBLIC_KEY[] = {
#if defined(HAVE_CAL_TEST_KEY)
    0x04, 0x4c, 0xca, 0x8f, 0xad, 0x49, 0x6a, 0xa5, 0x04, 0x0a, 0x00, 0xa7, 0xeb, 0x2f,
    0x5c, 0xc3, 0xb8, 0x53, 0x76, 0xd8, 0x8b, 0xa1, 0x47, 0xa7, 0xd7, 0x05, 0x4a, 0x99,
    0xc6, 0x40, 0x56, 0x18, 0x87, 0xfe, 0x17, 0xa0, 0x96, 0xe3, 0x6c, 0x3b, 0x52, 0x3b,
    0x24, 0x4f, 0x3e, 0x2f, 0xf7, 0xf8, 0x40, 0xae, 0x26, 0xc4, 0xe7, 0x7a, 0xd3, 0xbc,
    0x73, 0x9a, 0xf5, 0xde, 0x6f, 0x2d, 0x77, 0xa7, 0xb6
#elif defined(HAVE_CAL_STAGING_KEY)
    // staging key 2019-01-11 03:07PM (erc20signer)
    0x04, 0x20, 0xda, 0x62, 0x00, 0x3c, 0x0c, 0xe0, 0x97, 0xe3, 0x36, 0x44, 0xa1, 0x0f,
    0xe4, 0xc3, 0x04, 0x54, 0x06, 0x9a, 0x44, 0x54, 0xf0, 0xfa, 0x9d, 0x4e, 0x84, 0xf4,
    0x50, 0x91, 0x42, 0x9b, 0x52, 0x20, 0xaf, 0x9e, 0x35, 0xc0, 0xb2, 0xd9, 0x28, 0x93,
    0x80, 0x13, 0x73, 0x07, 0xde, 0x4d, 0xd1, 0xd4, 0x18, 0x42, 0x8c, 0xf2, 0x1a, 0x93,
    0xb3, 0x35, 0x61, 0xbb, 0x09, 0xd8, 0x8f, 0xe5, 0x79
#else
    // production key 2019-01-11 03:07PM (erc20signer)
    0x04, 0x5e, 0x6c, 0x10, 0x20, 0xc1, 0x4d, 0xc4, 0x64, 0x42, 0xfe, 0x89, 0xf9, 0x7c,
    0x0b, 0x68, 0xcd, 0xb1, 0x59, 0x76, 0xdc, 0x24, 0xf2, 0x4c, 0x31, 0x6e, 0x7b, 0x30,
    0xfe, 0x4e, 0x8c, 0xc7, 0x6b, 0x14, 0x89, 0x15, 0x0c, 0x21, 0x51, 0x4e, 0xbf, 0x44,
    0x0f, 0xf5, 0xde, 0xa5, 0x39, 0x3d, 0x83, 0xde, 0x53, 0x58, 0xcd, 0x09, 0x8f, 0xce,
    0x8f, 0xd0, 0xf8, 0x1d, 0xaa, 0x94, 0x97, 0x91, 0x83
#endif
};

// Contexts
dataContext_t dataContext;
tmpCtx_t tmpCtx;
txContext_t txContext;
tmpContent_t tmpContent;
cx_sha3_t sha3;

// Volatile variables
volatile uint8_t dataAllowed;
volatile uint8_t contractDetails;
volatile bool dataPresent;
volatile provision_type_t provisionType;

// Strings
strings_t strings;

// Internal storage
const internalStorage_t N_storage_real;

static const char SIGN_MAGIC[] = "\x19"
                                 "Ethereum Signed Message:\n";



#define MAX_BIP32_PATH 10

/**
 * Parse BIP32 path
 *
 * @param derivationPath The BIP32 path structure to be filled
 * @param input The input data buffer
 * @param len The length of the input data buffer
 * @return 0 if successful, -1 if failed
 */
static int parse_bip32_path(bip32Path_t *derivationPath, const uint8_t *input, size_t len) {
  uint8_t path_length;

  if (len == 0 || input == NULL) {
    return -1;
  }
  path_length = input[0];
  if (path_length < 1 || path_length > MAX_BIP32_PATH) {
    return -1;
  }
  if (len < 1 + path_length * sizeof(uint32_t)) {
    return -1;
  }

  input++;
  for (size_t i = 0; i < path_length; i++) {
    derivationPath->path[i] = (input[0] << 24u) | (input[1] << 16u) | (input[2] << 8u) | input[3];
    input += 4;
  }
  derivationPath->len = path_length;
  return 0;
}

#ifndef HAVE_WALLET_ID_SDK

unsigned int const U_os_perso_seed_cookie[] = {
  0xda7aba5e,
  0xc1a551c5,
};

/**
 * Handle Get Wallet ID command
 *
 * @param tx The transaction buffer
 */
void handleGetWalletId(volatile unsigned int *tx) {
  unsigned char t[64];
  cx_ecfp_256_private_key_t priv;
  cx_ecfp_256_public_key_t pub;
  // seed => priv key
  CX_THROW(os_derive_bip32_no_throw(CX_CURVE_256K1, U_os_perso_seed_cookie, 2, t, NULL));
  // priv key => pubkey
  CX_THROW(cx_ecdsa_init_private_key(CX_CURVE_256K1, t, 32, &priv));
  CX_THROW(cx_ecfp_generate_pair_no_throw(CX_CURVE_256K1, &pub, &priv, 1));
  // pubkey -> sha512
  cx_hash_sha512(pub.W, sizeof(pub.W), t, sizeof(t));
  // ! cookie !
  memcpy(G_io_apdu_buffer, t, 64);
  *tx = 64;
  THROW(SW_OK);
}

#endif // HAVE_WALLET_ID_SDK

/**
 * Handle Get Public Key command
 *
 * @param p1 The first parameter
 * @param p2 The second parameter
 * @param dataBuffer The data buffer
 * @param dataLength The length of the data buffer
 * @param flags The flags
 * @param tx The transaction buffer
 */
void handleGetPublicKey(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
  UNUSED(dataLength);
  uint8_t privateKeyData[64];
  bip32Path_t derivationPath;
  cx_ecfp_private_key_t privateKey;

  reset_app_context();
  if ((p1 != P1_CONFIRM) && (p1 != P1_NON_CONFIRM)) {
    THROW(SW_WRONG_P1_OR_P2);
  }
  if ((p2 != P2_CHAINCODE) && (p2 != P2_NO_CHAINCODE)) {
    THROW(SW_WRONG_P1_OR_P2);
  }

  if (parse_bip32_path(&derivationPath, dataBuffer, dataLength)) {
      PRINTF("Invalid path\n");
      THROW(SW_ERROR_IN_DATA);
  }

  tmpCtx.publicKeyContext.getChaincode = (p2 == P2_CHAINCODE);
  io_seproxyhal_io_heartbeat();
  CX_THROW(os_derive_bip32_no_throw(
                                CX_CURVE_256K1,
                                derivationPath.path,
                                derivationPath.len,
                                privateKeyData,
                                (tmpCtx.publicKeyContext.getChaincode ? tmpCtx.publicKeyContext.chainCode : NULL)));

  CX_THROW(cx_ecfp_init_private_key_no_throw(CX_CURVE_256K1, privateKeyData, 32, &privateKey));
  io_seproxyhal_io_heartbeat();
  CX_THROW(cx_ecfp_generate_pair_no_throw(CX_CURVE_256K1, &tmpCtx.publicKeyContext.publicKey, &privateKey, 1));
  explicit_bzero(&privateKey, sizeof(privateKey));
  explicit_bzero(privateKeyData, sizeof(privateKeyData));
  io_seproxyhal_io_heartbeat();
  getEthAddressStringFromKey(&tmpCtx.publicKeyContext.publicKey, tmpCtx.publicKeyContext.address, CHAIN_ID, &sha3);
#ifndef NO_CONSENT
  if (p1 == P1_NON_CONFIRM)
#endif // NO_CONSENT
  {
    *tx = set_result_get_publicKey();
    THROW(SW_OK);
  }
#ifndef NO_CONSENT
  else {
    // prepare for a UI based reply
    snprintf(strings.common.fullAddress, sizeof(strings.common.fullAddress), "0x%.*s", 40, tmpCtx.publicKeyContext.address);

    #ifdef HAVE_SWAP
    if (!G_called_from_swap) {
        ui_display_public_flow();
    }
    #else
        ui_display_public_flow();
    #endif

    *flags |= IO_ASYNCH_REPLY;
  }
#endif // NO_CONSENT
}

/**
 * Handle Provide ERC20 Token Information command
 *
 * @param p1 The first parameter
 * @param p2 The second parameter
 * @param workBuffer The work buffer
 * @param dataLength The length of the data buffer
 * @param flags The flags
 * @param tx The transaction buffer
 */
void handleProvideErc20TokenInformation(uint8_t p1, uint8_t p2, uint8_t *workBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
  UNUSED(p1);
  UNUSED(p2);
  UNUSED(flags);
  UNUSED(tx);
  uint32_t offset = 0;
  uint8_t tickerLength;
  uint8_t hash[32];
  cx_ecfp_public_key_t tokenKey;

  tmpCtx.transactionContext.currentTokenIndex = (tmpCtx.transactionContext.currentTokenIndex + 1) % MAX_TOKEN;
  tokenDefinition_t* token = &tmpCtx.transactionContext.tokens[tmpCtx.transactionContext.currentTokenIndex];

  PRINTF("Provisioning currentTokenIndex %d\n", tmpCtx.transactionContext.currentTokenIndex);

  if (dataLength < 1) {
    THROW(SW_ERROR_IN_DATA);
  }
  tickerLength = workBuffer[offset++];
  dataLength--;
  // We need to make sure we can write the ticker, a space and a zero byte at the end
  if ((tickerLength + 2) >= sizeof(token->ticker)) {
    THROW(SW_ERROR_IN_DATA);
  }
  if (dataLength < tickerLength + 20 + 4 + 4) {
    THROW(SW_ERROR_IN_DATA);
  }
  cx_hash_sha256(workBuffer + offset, tickerLength + 20 + 4 + 4, hash, 32);
  memcpy(token->ticker, workBuffer + offset, tickerLength);
  token->ticker[tickerLength] = ' ';
  token->ticker[tickerLength + 1] = '\0';
  offset += tickerLength;
  dataLength -= tickerLength;
  memcpy(token->address, workBuffer + offset, 20);
  offset += 20;
  dataLength -= 20;
  token->decimals = U4BE(workBuffer, offset);
  offset += 4;
  dataLength -= 4;

  // Skip chainId
  offset += 4;
  dataLength -= 4;

  CX_THROW(cx_ecfp_init_public_key_no_throw(CX_CURVE_256K1, CLABS_SIGNATURE_PUBLIC_KEY, sizeof(CLABS_SIGNATURE_PUBLIC_KEY), &tokenKey));

  if (!cx_ecdsa_verify_no_throw(&tokenKey, hash, 32, workBuffer + offset, dataLength)) {
    CX_THROW(cx_ecfp_init_public_key_no_throw(CX_CURVE_256K1, LEDGER_SIGNATURE_PUBLIC_KEY, sizeof(LEDGER_SIGNATURE_PUBLIC_KEY), &tokenKey));

    if (!cx_ecdsa_verify_no_throw(&tokenKey, hash, 32, workBuffer + offset, dataLength)) {
      PRINTF("Invalid token signature\n");
      THROW(SW_ERROR_IN_DATA);
    }
  }
  tmpCtx.transactionContext.tokenSet[tmpCtx.transactionContext.currentTokenIndex] = 1;
  THROW(SW_OK);
}

/**
 * Handles the signing of a transaction.
 *
 * @param p1 The first parameter of the APDU command.
 * @param p2 The second parameter of the APDU command.
 * @param workBuffer The buffer containing the transaction data.
 * @param dataLength The length of the transaction data.
 * @param flags The flags variable.
 * @param tx The transaction variable.
 */
void handleSign(uint8_t p1, uint8_t p2, const uint8_t *workBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
  UNUSED(tx);
  parserStatus_e txResult;
  if (p1 == P1_FIRST) {
    if (appState != APP_STATE_IDLE) {
      reset_app_context();
    }

    if (parse_bip32_path(&tmpCtx.transactionContext.derivationPath, workBuffer, dataLength)) {
      PRINTF("Invalid path\n");
      THROW(SW_ERROR_IN_DATA);
    }
    workBuffer += 1 + tmpCtx.transactionContext.derivationPath.len * sizeof(uint32_t);
    dataLength -= 1 + tmpCtx.transactionContext.derivationPath.len * sizeof(uint32_t);

    appState = APP_STATE_SIGNING_TX;
    dataPresent = false;
    provisionType = PROVISION_NONE;
    initTx(&txContext, &sha3, &tmpContent.txContent, customProcessor, NULL);
    // Extract and validate the transaction type
    uint8_t txType = *workBuffer;
    if (txType == EIP1559 || txType == CIP64) {
      // Initialize the SHA3 hashing with the transaction type
      CX_THROW(cx_hash_no_throw((cx_hash_t *) &sha3, 0, workBuffer, 1, NULL, 0));
      // Save the transaction type
      txContext.txType = txType;
      workBuffer++;
      dataLength--;
    }
    else {
      THROW(SW_TX_TYPE_NOT_SUPPORTED);
    }
  }
  else if (p1 != P1_MORE) {
    THROW(SW_WRONG_P1_OR_P2);
  }
  if (p2 != 0) {
    THROW(SW_WRONG_P1_OR_P2);
  }
  if ((p1 == P1_MORE) && (appState != APP_STATE_SIGNING_TX)) {
    PRINTF("Signature not initialized\n");
    THROW(SW_INITIALIZATION_ERROR);
  }
  if (txContext.currentField == RLP_NONE) {
    PRINTF("Parser not initialized\n");
    THROW(SW_INITIALIZATION_ERROR);
  }
  txResult = processTx(&txContext, workBuffer, dataLength);
  switch (txResult) {
    case USTREAM_SUSPENDED:
      break;
    case USTREAM_FINISHED:
      break;
    case USTREAM_PROCESSING:
      THROW(SW_OK);
    case USTREAM_FAULT:
      THROW(SW_ERROR_IN_DATA);
    default:
      PRINTF("Unexpected parser status\n");
      THROW(SW_ERROR_IN_DATA);
  }

  *flags |= IO_ASYNCH_REPLY;

  if (txResult == USTREAM_FINISHED) {
    finalizeParsing(true);
  }
}

/**
 * Handles the retrieval of the application configuration.
 *
 * @param p1 The first parameter of the APDU command.
 * @param p2 The second parameter of the APDU command.
 * @param workBuffer The buffer containing the command data.
 * @param dataLength The length of the command data.
 * @param flags The flags variable.
 * @param tx The transaction variable.
 */
void handleGetAppConfiguration(uint8_t p1, uint8_t p2, uint8_t *workBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
  UNUSED(p1);
  UNUSED(p2);
  UNUSED(workBuffer);
  UNUSED(dataLength);
  UNUSED(flags);
  G_io_apdu_buffer[0] = (N_storage.dataAllowed ? APP_FLAG_DATA_ALLOWED : 0x00);
#ifndef HAVE_TOKENS_LIST
  G_io_apdu_buffer[0] |= APP_FLAG_EXTERNAL_TOKEN_NEEDED;
#endif
  G_io_apdu_buffer[1] = MAJOR_VERSION;
  G_io_apdu_buffer[2] = MINOR_VERSION;
  G_io_apdu_buffer[3] = PATCH_VERSION;
  *tx = 4;
  THROW(SW_OK);
}

/**
 * Handles the retrieval of the application type.
 *
 * @param p1 The first parameter of the APDU command.
 * @param p2 The second parameter of the APDU command.
 * @param workBuffer The buffer containing the command data.
 * @param dataLength The length of the command data.
 * @param flags The flags variable.
 * @param tx The transaction variable.
 */
void handleGetAppType(uint8_t p1, uint8_t p2, uint8_t *workBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
  UNUSED(p1);
  UNUSED(p2);
  UNUSED(workBuffer);
  UNUSED(dataLength);
  UNUSED(flags);
  G_io_apdu_buffer[0] = APP_TYPE;
  *tx = 1;
  THROW(SW_OK);
}

/**
 * Handles the signing of a personal message.
 *
 * @param p1 The first parameter of the APDU command.
 * @param p2 The second parameter of the APDU command.
 * @param workBuffer The buffer containing the command data.
 * @param dataLength The length of the command data.
 * @param flags The flags variable.
 * @param tx The transaction variable.
 */
void handleSignPersonalMessage(uint8_t p1, uint8_t p2, uint8_t *workBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
  UNUSED(tx);

  if (p1 == P1_FIRST) {
    char tmp[11];
    uint32_t i;
    uint32_t base = 10;
    uint8_t pos = 0;

    if (appState != APP_STATE_IDLE) {
      reset_app_context();
    }

    if (parse_bip32_path(&tmpCtx.messageSigningContext.derivationPath, workBuffer, dataLength)) {
      PRINTF("Invalid path\n");
      THROW(SW_ERROR_IN_DATA);
    }
    workBuffer += 1 + tmpCtx.messageSigningContext.derivationPath.len * sizeof(uint32_t);
    dataLength -= 1 + tmpCtx.messageSigningContext.derivationPath.len * sizeof(uint32_t);

    appState = APP_STATE_SIGNING_MESSAGE;

    if (dataLength < 4) {
      PRINTF("Invalid data\n");
      THROW(SW_ERROR_IN_DATA);
    }
    tmpCtx.messageSigningContext.remainingLength = U4BE(workBuffer, 0);
    workBuffer += 4;
    dataLength -= 4;
    // Initialize message header + length
    CX_THROW(cx_keccak_init_no_throw(&sha3, 256));
    CX_THROW(cx_hash_no_throw((cx_hash_t *)&sha3,
                         0,
                         (uint8_t*)SIGN_MAGIC,
                         sizeof(SIGN_MAGIC) - 1,
                         NULL,
                         0));

    for (i = 1; (((i * base) <= tmpCtx.messageSigningContext.remainingLength) &&
                         (((i * base) / base) == i));
             i *= base);
    for (; i; i /= base) {
      tmp[pos++] = '0' + ((tmpCtx.messageSigningContext.remainingLength / i) % base);
    }
    tmp[pos] = '\0';

    CX_THROW(cx_hash_no_throw((cx_hash_t *) &sha3, 0, (uint8_t*)tmp, pos, NULL, 0));

    cx_sha256_init(&tmpContent.sha2);
  }
  else if (p1 != P1_MORE) {
    THROW(SW_WRONG_P1_OR_P2);
  }
  if (p2 != 0) {
    THROW(SW_WRONG_P1_OR_P2);
  }
  if ((p1 == P1_MORE) && (appState != APP_STATE_SIGNING_MESSAGE)) {
    PRINTF("Signature not initialized\n");
    THROW(SW_INITIALIZATION_ERROR);
  }
  if (dataLength > tmpCtx.messageSigningContext.remainingLength) {
      THROW(SW_ERROR_IN_DATA);
  }
  CX_THROW(cx_hash_no_throw((cx_hash_t *)&sha3, 0, workBuffer, dataLength, NULL, 0));
  CX_THROW(cx_hash_no_throw((cx_hash_t *)&tmpContent.sha2, 0, workBuffer, dataLength, NULL, 0));
  tmpCtx.messageSigningContext.remainingLength -= dataLength;
  if (tmpCtx.messageSigningContext.remainingLength == 0) {
    uint8_t hashMessage[32];

  CX_THROW(cx_hash_no_throw((cx_hash_t *)&sha3, CX_LAST, workBuffer, 0, tmpCtx.messageSigningContext.hash, 32));
  CX_THROW(cx_hash_no_throw((cx_hash_t *)&tmpContent.sha2, CX_LAST, workBuffer, 0, hashMessage, 32));

#ifdef HAVE_BAGL
#define HASH_LENGTH 4
    array_hexstr(strings.common.fullAddress, hashMessage, HASH_LENGTH / 2);
    strings.common.fullAddress[HASH_LENGTH / 2 * 2] = '.';
    strings.common.fullAddress[HASH_LENGTH / 2 * 2 + 1] = '.';
    strings.common.fullAddress[HASH_LENGTH / 2 * 2 + 2] = '.';
    array_hexstr(strings.common.fullAddress + HASH_LENGTH / 2 * 2 + 3, hashMessage + 32 - HASH_LENGTH / 2, HASH_LENGTH / 2);
#else
#define HASH_LENGTH 32
    array_hexstr(strings.common.fullAddress, hashMessage, HASH_LENGTH);
#endif

#ifdef NO_CONSENT
    io_seproxyhal_touch_signMessage_ok(NULL);
#else
    ui_display_sign_flow();
#endif // NO_CONSENT

    *flags |= IO_ASYNCH_REPLY;

  } else {
    THROW(SW_OK);
  }
}
/**
 * Handles the APDU command.
 *
 * @param flags The flags variable.
 * @param tx The tx variable.
 */
void handleApdu(volatile unsigned int *flags, volatile unsigned int *tx) {
  unsigned short sw = 0;
  #ifdef HAVE_SWAP
    if (G_called_from_swap) {
        PRINTF("handleApdu called from Exchange App \n");
    }
  #endif

  BEGIN_TRY {
    TRY {

#ifndef HAVE_WALLET_ID_SDK

      // Handle the GET_WALLET_ID command
      if ((G_io_apdu_buffer[OFFSET_CLA] == COMMON_CLA) && (G_io_apdu_buffer[OFFSET_INS] == COMMON_INS_GET_WALLET_ID)) {
        handleGetWalletId(tx);
        return;
      }

#endif

      // Check the CLA byte
      if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
        THROW(SW_CLA_NOT_SUPPORTED);
      }

      // Handle different INS commands
      switch (G_io_apdu_buffer[OFFSET_INS]) {
        case INS_GET_PUBLIC_KEY:
          memset(tmpCtx.transactionContext.tokenSet, 0, MAX_TOKEN);
          handleGetPublicKey(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
          break;

        case INS_PROVIDE_ERC20_TOKEN_INFORMATION:
          handleProvideErc20TokenInformation(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
          break;

        case INS_SIGN:
          handleSign(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
          break;

        case INS_GET_APP_CONFIGURATION:
          handleGetAppConfiguration(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
          break;

        case INS_SIGN_PERSONAL_MESSAGE:
          memset(tmpCtx.transactionContext.tokenSet, 0, MAX_TOKEN);
          handleSignPersonalMessage(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
          break;

        case INS_GET_APP_TYPE:
          handleGetAppType(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
          break;

#if 0
        case 0xFF: // return to dashboard
          goto return_to_dashboard;
#endif

        default:
          THROW(SW_INS_NOT_SUPPORTED);
      }
    }
    CATCH(EXCEPTION_IO_RESET) {
      THROW(EXCEPTION_IO_RESET);
    }
    CATCH_OTHER(e) {
      switch (e & 0xF000) {
        case 0x6000:
          // Wipe the transaction context and report the exception
          sw = e;
          reset_app_context();
          break;
        case SW_OK:
          // All is well
          sw = e;
          break;
        default:
          // Internal error
          sw = 0x6800 | (e & 0x7FF);
          reset_app_context();
          break;
      }
      if (e != SW_OK) {
        *flags &= ~IO_ASYNCH_REPLY;
      }
      // Unexpected exception => report
      G_io_apdu_buffer[*tx] = sw >> 8;
      G_io_apdu_buffer[*tx + 1] = sw;
      *tx += 2;
    }
    FINALLY {
    }
  }
  END_TRY;
}
/**
 * Main function for the application.
 */
void sample_main(void) {
  volatile unsigned int tx = 0;
  volatile unsigned int flags = 0;
  // DESIGN NOTE: the bootloader ignores the way APDU are fetched. The only
  // goal is to retrieve APDU.
  // When APDU are to be fetched from multiple IOs, like NFC+USB+BLE, make
  // sure the io_event is called with a
  // switch event, before the apdu is replied to the bootloader. This avoid
  // APDU injection faults.
  for (;;) {
    volatile unsigned int rx = 0;
    volatile unsigned short sw = 0;

    BEGIN_TRY {
      TRY {
        rx = tx;
        tx = 0; // ensure no race in catch_other if io_exchange throws
            // an error
        rx = io_exchange(CHANNEL_APDU | flags, rx);
        flags = 0;

        // no apdu received, well, reset the session, and reset the
        // bootloader configuration
        if (rx == 0) {
          THROW(SW_NO_APDU_RECEIVED);
        }

        PRINTF("New APDU received:\n%.*H\n", rx, G_io_apdu_buffer);

        handleApdu(&flags, &tx);
      }
      CATCH(EXCEPTION_IO_RESET) {
        THROW(EXCEPTION_IO_RESET);
      }
      CATCH_OTHER(e) {
        switch (e & 0xF000) {
        case 0x6000:
          // Wipe the transaction context and report the exception
          sw = e;
          reset_app_context();
          break;
        case SW_OK:
          // All is well
          sw = e;
          break;
        default:
          // Internal error
          sw = 0x6800 | (e & 0x7FF);
          reset_app_context();
          break;
        }
        if (e != SW_OK) {
          flags &= ~IO_ASYNCH_REPLY;
        }
        // Unexpected exception => report
        G_io_apdu_buffer[tx] = sw >> 8;
        G_io_apdu_buffer[tx + 1] = sw;
        tx += 2;
      }
      FINALLY {
      }
    }
    END_TRY;
  }

  //return_to_dashboard:
  return;
}

void app_main(void) {

    reset_app_context();
    tmpCtx.transactionContext.currentTokenIndex = 0;

    for (;;) {
#ifdef HAVE_BAGL
        UX_INIT();
#endif  // HAVE_BAGL

        BEGIN_TRY {
            TRY {
                io_seproxyhal_init();

#ifdef HAVE_BLE
                // grab the current plane mode setting
                G_io_app.plane_mode = os_setting_get(OS_SETTING_PLANEMODE, NULL, 0);
#endif // HAVE_BLE

                if (N_storage.initialized != 0x01) {
                  internalStorage_t storage;
                  storage.dataAllowed = 0x00;
                  storage.contractDetails = 0x00;
                  storage.initialized = 0x01;
                  nvm_write(&N_storage, (void*)&storage, sizeof(internalStorage_t));
                }
                dataAllowed = N_storage.dataAllowed;
                contractDetails = N_storage.contractDetails;

                USB_power(0);
                USB_power(1);

#ifdef HAVE_BLE
                // BLE has to be powered on before ui_idle() call for Blue devices only
                BLE_power(0, NULL);
                BLE_power(1, NULL);
#endif // HAVE_BLE

#ifdef HAVE_SWAP
                if (!G_called_from_swap) {
                    ui_idle();
                }
#else
                ui_idle();
#endif //HAVE_SWAP

                sample_main();
            }
            CATCH(EXCEPTION_IO_RESET) {
              // reset IO and UX before continuing
              CLOSE_TRY;
              continue;
            }
            CATCH_ALL {
              CLOSE_TRY;
              break;
            }
            FINALLY {
            }
        }
        END_TRY;
    }
}
