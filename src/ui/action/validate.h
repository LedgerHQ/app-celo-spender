#pragma once

#include <stdbool.h>  // bool

/**
 * Action for public key validation and export.
 *
 * @param[in] choice
 *   User choice (either approved or rejected).
 *
 */
void validate_pubkey(bool choice);

/**
 * Action for signing a message.
 *
 * @param[in] confirm
 *   User confirmation (either approved or rejected).
 *
 */
int crypto_sign_message(bool confirm);

/**
 * Action for signing a transaction.
 *
 * @param[in] confirm
 *   User confirmation (either approved or rejected).
 *
 */
int validate_transaction(bool confirm);