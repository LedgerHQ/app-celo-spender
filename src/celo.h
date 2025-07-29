/**
 * @file celo.h
 * @brief Header file containing function declarations and definitions for the Celo application.
 */

#pragma once

#include <stdint.h>
#include "ethUstream.h"
#include "tokens.h"

/**
 * @brief Sends the status code to the SE proxy hardware abstraction layer.
 *
 * @param sw The status code to be sent.
 */
void io_seproxyhal_send_status(uint32_t sw);

/**
 * @brief Formats the signature output.
 *
 * @param signature The signature to be formatted.
 */
void format_signature_out(const uint8_t* signature);

/**
 * @brief Sets the result and retrieves the public key.
 *
 * @return The status code for the operation.
 */
uint32_t set_result_get_publicKey();

/**
 * @brief Resets the application context.
 */
void reset_app_context();

/**
 * @brief Retrieves the known token based on the token address.
 *
 * @param tokenAddr The address of the token.
 * @return A pointer to the token definition.
 */
tokenDefinition_t* getKnownToken(uint8_t *tokenAddr);

/**
 * @brief Custom processor for transaction context.
 *
 * @param context The transaction context.
 * @return The custom status code.
 */
customStatus_e customProcessor(txContext_t *context);

/**
 * @brief Initializes the transaction context.
 *
 * @param context The transaction context.
 * @param sha3 The SHA3 context.
 * @param content The transaction content.
 * @param customProcessor The custom processor function.
 * @param extra Additional data for the custom processor.
 */
void initTx(txContext_t *context, cx_sha3_t *sha3, txContent_t *content, ustreamProcess_t customProcessor, void *extra);

/**
 * @brief Finalizes the parsing process.
 *
 * @param direct Flag indicating if the parsing is direct.
 */
void finalizeParsing(bool direct);

/**
 * @brief Displays the transaction on the user interface.
 */
void ui_display_transaction();

// TODO: this should not be exposed
/**
 * @brief Enumeration representing the application state.
 */
typedef enum {
  APP_STATE_IDLE,
  APP_STATE_SIGNING_TX,
  APP_STATE_SIGNING_MESSAGE
} app_state_t;

extern volatile uint8_t appState; /**< The application state. */
