#include <stddef.h>
#include <stdint.h>
#include "constants.h"
#include "globals.h"
#include "sw.h"
#include "send_response.h"
#include "io.h"

int helper_send_response_pubkey() {
  uint8_t resp[1 + PUBKEY_LEN + 1 + CHAINCODE_LEN] = {0};
  size_t offset = 0;
  resp[offset++] = PUBKEY_LEN;
  memcpy(resp + offset, tmpCtx.publicKeyContext.publicKey.W, PUBKEY_LEN);
  offset += PUBKEY_LEN;
  resp[offset++] = CHAINCODE_LEN;
  memcpy(resp + offset, tmpCtx.publicKeyContext.address, CHAINCODE_LEN);
  offset += 40;
  if (tmpCtx.publicKeyContext.getChaincode) {
    memcpy(resp + offset, tmpCtx.publicKeyContext.chainCode, CHAINCODE_LEN);
    offset += CHAINCODE_LEN;
  }
  return io_send_response_pointer(resp, offset, SW_OK);
}
