#pragma once

#include <stdint.h>
#include "ethUstream.h"
#include "tokens.h"

void io_seproxyhal_send_status(uint32_t sw);
void format_signature_out(const uint8_t* signature);
uint32_t set_result_get_publicKey();
void reset_app_context();

tokenDefinition_t* getKnownToken(uint8_t *tokenAddr);

customStatus_e customProcessor(txContext_t *context);
void initTx(txContext_t *context, cx_sha3_t *sha3, txContent_t *content, ustreamProcess_t customProcessor, void *extra);
void finalizeParsing(bool direct);

// TODO: this should not be exposed
typedef enum {
  APP_STATE_IDLE,
  APP_STATE_SIGNING_TX,
  APP_STATE_SIGNING_MESSAGE
} app_state_t;

extern volatile uint8_t appState;
