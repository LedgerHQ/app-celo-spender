/**
 * @file swap_validation.h
 * @brief What swap mode is allowed to sign.
 *
 * In swap mode the app signs with no review at all, so this comparison is the
 * only thing standing between Exchange's approval and a signature. It is kept
 * free of SDK and global state so that the whole decision table can be
 * exercised on the host: see tests/unit/test_swap_validation.c.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "celo.h"
#include "ethUstream.h"

/** @brief The transaction parameters Exchange approved. */
typedef struct swap_approved_t {
    const char *ticker;    /**< Asset ticker, as Exchange's coin configuration gives it. */
    uint8_t decimals;      /**< That asset's decimals. */
    const char *recipient; /**< Approved recipient, "0x"-prefixed. */
    const uint8_t *amount; /**< Approved amount, big-endian. */
    uint8_t amount_length;
    const uint8_t *fee; /**< Approved fee ceiling, big-endian. */
    uint8_t fee_length;
} swap_approved_t;

/** @brief What the parser made of the transaction now waiting to be signed. */
typedef struct swap_tx_t {
    const char *ticker; /**< Asset the app resolved for display; may carry a leading space. */
    uint8_t decimals;
    provision_type_t provisionType; /**< How the calldata was classified. */
    const uint8_t *selector;        /**< Stored calldata selector; read only for PROVISION_TOKEN. */
    bool dataPresent;               /**< Whether the transaction carries calldata. */
    const char *destination;        /**< Recipient, "0x"-prefixed, after any calldata rewrite. */
    uint8_t destinationLength;      /**< Raw destination length; 0 means contract creation. */
    const txInt256_t *value;
    const txInt256_t *gasprice;
    const txInt256_t *startgas;
    uint8_t feeCurrencyLength; /**< Non-zero means fees are paid in a token. */
} swap_tx_t;

/**
 * @brief Decides whether a transaction may be signed against an approval.
 *
 * @param tx What the parser produced.
 * @param approved What Exchange approved.
 * @return true only if every bound property matches.
 */
bool swap_tx_matches_approval(const swap_tx_t *tx, const swap_approved_t *approved);
