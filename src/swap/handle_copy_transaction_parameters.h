#ifdef HAVE_SWAP
#include <string.h>  // memset, explicit_bzero
#include "swap.h"

bool swap_copy_transaction_parameters(create_transaction_parameters_t *sign_transaction_params);

/**
 * @brief Checks the parsed transaction against what Exchange approved.
 *
 * Swap mode signs with no review, so this is the only gate. The asset cannot be
 * recovered from the transaction by the time it runs -- finalizeParsing() has
 * already replaced destination and value with the ones taken from the token
 * calldata -- so the caller passes what it resolved.
 *
 * @param ticker The asset being transferred, as the app resolved it for display.
 * @param decimals That asset's decimals.
 * @return true if the transaction may be signed without review.
 */
bool swap_check_validity(const char *ticker, uint8_t decimals);

void swap_finalize_exchange_sign_transaction(bool is_success);

#endif  // HAVE_SWAP