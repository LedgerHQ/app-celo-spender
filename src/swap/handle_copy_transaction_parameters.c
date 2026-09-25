#ifdef HAVE_SWAP
#include "handle_copy_transaction_parameters.h"

#include "celo.h"
#include "ethUtils.h"
#include "globals.h"
#include "swap_validation.h"
#include "utils.h"

typedef struct swap_validated_s {
    bool initialized;
    uint8_t decimals;
    char ticker[MAX_TICKER_LEN];
    uint8_t amount[32];
    uint8_t amount_length;
    uint8_t fee[32];
    uint8_t fee_length;
    char recipient[ADDRESS_LENGTH + 2];  // +2 for "0x" prefix
} swap_validated_t;

static swap_validated_t G_swap_validated;

// Save the BSS address where we will write the return value when finished
static uint8_t* G_swap_sign_return_value_address;

bool swap_copy_transaction_parameters(create_transaction_parameters_t* params) {
    PRINTF("Inside Celo swap_copy_transaction_parameters\n");
    // Ensure no extraid
    if (params->destination_address_extra_id == NULL) {
        PRINTF("destination_address_extra_id expected\n");
        return false;
    } else if (params->destination_address_extra_id[0] != '\0') {
        PRINTF("destination_address_extra_id expected empty, not '%s'\n",
               params->destination_address_extra_id);
        return false;
    }

    if (params->destination_address == NULL) {
        PRINTF("Destination address expected\n");
        return false;
    }

    if (params->amount == NULL) {
        PRINTF("Amount expected\n");
        return false;
    }

    // first copy parameters to stack, and then to global data.
    // We need this "trick" as the input data position can overlap with app
    // globals
    swap_validated_t swap_validated;
    explicit_bzero(&swap_validated, sizeof(swap_validated));

    // Parse config and save decimals and ticker
    // If there is no coin_configuration, consider that we are doing a CELO swap
    if (params->coin_configuration == NULL) {
        memcpy(swap_validated.ticker, "CELO", sizeof("CELO"));
        swap_validated.decimals = CELO_PRECISION;
    } else {
        if (!swap_parse_config(
                params->coin_configuration, params->coin_configuration_length,
                swap_validated.ticker, sizeof(swap_validated.ticker),
                &swap_validated.decimals)) {
            PRINTF("Fail to parse coin_configuration\n");
            return false;
        }
    }
    // Save recipient
    strlcpy(swap_validated.recipient, params->destination_address,
            sizeof(swap_validated.recipient));
    if (swap_validated.recipient[sizeof(swap_validated.recipient) - 1] !=
        '\0') {
        PRINTF("Address copy error\n");
        return false;
    }

    // Save amount
    swap_validated.amount_length = params->amount_length;
    memcpy(swap_validated.amount, params->amount, params->amount_length);
    swap_validated.fee_length = params->fee_amount_length;

    memcpy(swap_validated.fee, params->fee_amount, params->fee_amount_length);
    swap_validated.initialized = true;

    // Full reset the global variables
    os_explicit_zero_BSS_segment();

    // Keep the address at which we'll reply the signing status
    G_swap_sign_return_value_address = &params->result;
    // Commit the values read from exchange to the clean global space
    memcpy(&G_swap_validated, &swap_validated, sizeof(swap_validated));
    PRINTF("Exiting Celo swap_copy_transaction_parameters\n");
    return true;
}

bool swap_check_validity(const char* ticker, uint8_t decimals) {
    PRINTF("Inside CELO swap_check_validity\n");
    if (!G_swap_validated.initialized) {
        PRINTF("Swap Validated data not initialized.\n");
        return false;
    }

    // The address comparison is done on the checksummed string, so it has to be
    // rendered before the decision can be made.
    char address[ADDRESS_LENGTH];
    char fullAddress[ADDRESS_LENGTH + 2];
    getEthAddressStringFromBinary(tmpContent.txContent.destination, address,
                                  CHAIN_ID, &sha3);
    fullAddress[0] = '0';
    fullAddress[1] = 'x';
    memcpy(fullAddress + 2, address, 40);
    fullAddress[42] = '\0';

    const swap_tx_t tx = {
        .ticker = ticker,
        .decimals = decimals,
        .provisionType = provisionType,
        .selector = dataContext.tokenContext.data,
        .dataPresent = dataPresent,
        .destination = fullAddress,
        .destinationLength = tmpContent.txContent.destinationLength,
        .value = &tmpContent.txContent.value,
        .gasprice = &tmpContent.txContent.gasprice,
        .startgas = &tmpContent.txContent.startgas,
        .feeCurrencyLength = tmpContent.txContent.feeCurrencyLength,
    };
    const swap_approved_t approved = {
        .ticker = G_swap_validated.ticker,
        .decimals = G_swap_validated.decimals,
        .recipient = G_swap_validated.recipient,
        .amount = G_swap_validated.amount,
        .amount_length = G_swap_validated.amount_length,
        .fee = G_swap_validated.fee,
        .fee_length = G_swap_validated.fee_length,
    };

    if (!swap_tx_matches_approval(&tx, &approved)) {
        PRINTF("Transaction does not match what Exchange approved\n");
        return false;
    }
    return true;
}

void swap_finalize_exchange_sign_transaction(bool is_success) {
    PRINTF("Returning to Exchange with status %d\n", is_success);
    // reset_app_context() must be called BEFORE writing the result.
    // On our BSS layout, tmpContent (cleared by reset_app_context) overlaps
    // Exchange's lib_in_out_params->result in shared SRAM. Writing result=1
    // first and then zeroing tmpContent would overwrite the flag with 0,
    // causing Exchange to see a failed swap even after a successful sign.
    reset_app_context();
    *G_swap_sign_return_value_address = is_success ? 1 : 0;
    os_lib_end();
}
#endif  // HAVE_SWAP