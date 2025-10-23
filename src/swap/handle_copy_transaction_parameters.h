#ifdef HAVE_SWAP
#include <string.h>  // memset, explicit_bzero
#include "swap.h"


bool swap_copy_transaction_parameters(create_transaction_parameters_t *sign_transaction_params);

bool swap_check_validity();

void swap_finalize_exchange_sign_transaction(bool is_success); 

#endif // HAVE_SWAP