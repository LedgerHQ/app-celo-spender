#include "provide_erc20_token_information.h"

#include "io.h"
#include "sw.h"
#include "globals.h"
#include "constants.h"
#include "celo.h"
#include "utils.h"
#include "ethUtils.h"
#include "ui_common.h"

// Token signature public key
uint8_t const TOKEN_SIGNATURE_PUBLIC_KEY[] = {
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

    PRINTF(
        "km_logs [provide_erc20_token_information.c] (handleProvideErc20TokenInformation) - START "
        "tmpCtx.transactionContext.extraInfo: ");
    for (int i = 0; i < MAX_ASSETS; i++) {
        PRINTF("\n[%d].address: ", i);
        for (int j = 0; j < ADDRESS_LENGTH; j++) {
            PRINTF("%02x", tmpCtx.transactionContext.extraInfo[i].token.address[j]);
        }
    }
    PRINTF("\n");

    tokenDefinition_t *token =
        &tmpCtx.transactionContext.extraInfo[tmpCtx.transactionContext.currentAssetIndex].token;

    PRINTF("Provisioning currentAssetIndex %d\n", tmpCtx.transactionContext.currentAssetIndex);

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
    CX_THROW(cx_ecfp_init_public_key_no_throw(CX_CURVE_256K1,
                                              TOKEN_SIGNATURE_PUBLIC_KEY,
                                              sizeof(TOKEN_SIGNATURE_PUBLIC_KEY),
                                              &tokenKey));
  if (!cx_ecdsa_verify_no_throw(&tokenKey, hash, 32, workBuffer + offset, dataLength)) {
    PRINTF("Invalid token signature\n");
    THROW(SW_ERROR_IN_DATA);
  }
    PRINTF(
        "km_logs [provide_erc20_token_information.c] (handleProvideErc20TokenInformation) - END "
        "tmpCtx.transactionContext.extraInfo: ");
    for (int i = 0; i < MAX_ASSETS; i++) {
        PRINTF("\n[%d].address: ", i);
        for (int j = 0; j < ADDRESS_LENGTH; j++) {
            PRINTF("%02x", tmpCtx.transactionContext.extraInfo[i].token.address[j]);
        }
    }
    PRINTF("\n");

    tmpCtx.transactionContext.assetSet[tmpCtx.transactionContext.currentAssetIndex] = 1;
    tmpCtx.transactionContext.currentAssetIndex =
        (tmpCtx.transactionContext.currentAssetIndex + 1) % MAX_ASSETS;
  THROW(SW_OK);
}
