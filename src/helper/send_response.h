#pragma once

/**
 * Helper to send APDU response with public key and chain code.
 *
 * response = PUBKEY_LEN (1) ||
 *            public_key (PUBKEY_LEN) ||
 *            CHAINCODE_LEN (1) ||
 *            chain_code (CHAINCODE_LEN)
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
int helper_send_response_pubkey(void);

/**
 * Helper to send APDU response with signature and v (parity of
 * y-coordinate of R).
 *
 * response = v (1) ||
 *            r (32) ||
 *            s (32)
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
int helper_send_response_signature(const uint8_t *signature, unsigned int info, bool sign_message);