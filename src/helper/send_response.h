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
 * response = signature_len (1) ||
 *            signature (signature_len) ||
 *            v (1)
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
int helper_send_response_sig(void);
