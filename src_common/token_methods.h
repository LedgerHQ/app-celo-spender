/**
 * @file token_methods.h
 * @brief ABI selectors of the token transfer methods the app clear-signs.
 *
 * These two live apart from the staking selectors in celo.c because two layers
 * need them: the parser, to classify calldata, and the swap gate, to decide what
 * may be signed without review. Keeping them free of SDK dependencies lets that
 * decision be tested on the host.
 */

#ifndef _TOKEN_METHODS_H_
#define _TOKEN_METHODS_H_

#include <stdint.h>

/** @brief Length of an ABI function selector. */
#define SELECTOR_LEN 4

/** @brief transfer(address to, uint256 value) */
extern const uint8_t TOKEN_TRANSFER_ID[SELECTOR_LEN];

/** @brief transferWithComment(address to, uint256 value, string comment) */
extern const uint8_t TOKEN_TRANSFER_WITH_COMMENT_ID[SELECTOR_LEN];

#endif /* _TOKEN_METHODS_H_ */
