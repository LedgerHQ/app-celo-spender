#include "sign_personal_message.h"

#include "io.h"
#include "sw.h"
#include "os.h"
#include "ui_common.h"
#include "globals.h"
#include "constants.h"
#include "celo.h"
#include "utils.h"
#include "ethUtils.h"

static const char SIGN_MAGIC[] = "\x19"
                                 "Ethereum Signed Message:\n";

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
