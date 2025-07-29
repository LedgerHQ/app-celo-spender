#ifdef HAVE_SWAP
#include "handle_copy_transaction_parameters.h"
#include "globals.h"
#include "utils.h"
#include "ethUtils.h"
#include "celo.h"

typedef struct swap_validated_s {
    bool initialized;
    uint8_t decimals;
    char ticker[MAX_TICKER_LEN];
    uint8_t amount[32];
    uint8_t amount_length;
    uint8_t fee[32];
    uint8_t fee_length;
    char recipient[ADDRESS_LENGTH+2]; // +2 for "0x" prefix
} swap_validated_t;

static swap_validated_t G_swap_validated;

// Save the BSS address where we will write the return value when finished
static uint8_t *G_swap_sign_return_value_address;

bool swap_copy_transaction_parameters(create_transaction_parameters_t *params) {
  
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
    // We need this "trick" as the input data position can overlap with app globals
    swap_validated_t swap_validated;
    memset(&swap_validated, 0, sizeof(swap_validated));

    // Parse config and save decimals and ticker
    // If there is no coin_configuration, consider that we are doing a CELO swap
    if (params->coin_configuration == NULL) {
        memcpy(swap_validated.ticker, "CELO", sizeof("CELO"));
        swap_validated.decimals = CELO_PRECISION;
    } else {
        if (!swap_parse_config(params->coin_configuration,
                               params->coin_configuration_length,
                               swap_validated.ticker,
                               sizeof(swap_validated.ticker),
                               &swap_validated.decimals)) {
            PRINTF("Fail to parse coin_configuration\n");
            return false;
        }
        
    }
    //Save recipient
    strlcpy(swap_validated.recipient,
            params->destination_address,
            sizeof(swap_validated.recipient));
    if (swap_validated.recipient[sizeof(swap_validated.recipient) - 1] != '\0') {
        PRINTF("Address copy error\n");
        return false;
    }
    
    // Save amount
    swap_validated.amount_length = params->amount_length;
    if (!memcpy(swap_validated.amount, params->amount, params->amount_length)) {
        return false;
    }
    // Save fees
    swap_validated.fee_length = params->fee_amount_length;
    
    if (!memcpy(swap_validated.fee, params->fee_amount, params->fee_amount_length)) {
        return false;
    }
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

bool swap_check_validity() {
    PRINTF("Inside CELO swap_check_validity\n");
    if (!G_swap_validated.initialized) {
        PRINTF("Swap Validated data not initialized.\n");
        return false;
    }

    // recipient
    char address[ADDRESS_LENGTH];
    char fullAddress[ADDRESS_LENGTH + 2];
    getEthAddressStringFromBinary(tmpContent.txContent.destination, address, CHAIN_ID, &sha3);
    fullAddress[0] = '0';
    fullAddress[1] = 'x';
    memcpy(fullAddress+2, address, 40);
    fullAddress[42] = '\0';
    if (_strcasecmp(fullAddress, G_swap_validated.recipient) != 0) {
        PRINTF("Recipient address does not match\n");
        return false;
    }

    // amount
    if (tmpContent.txContent.value.length != G_swap_validated.amount_length) {
        PRINTF("Amount length does not match \n");
        return false;
    }
    if (memcmp(tmpContent.txContent.value.value, G_swap_validated.amount, G_swap_validated.amount_length) != 0) {
        PRINTF("Amount does not match \n");
        return false;
    }
    // fee
    uint256_t gasPrice, startGas, feesRes, feesValidatedRes;
    convertUint256BE(tmpContent.txContent.gasprice.value, tmpContent.txContent.gasprice.length, &gasPrice);
    convertUint256BE(tmpContent.txContent.startgas.value, tmpContent.txContent.startgas.length, &startGas);
    mul256(&gasPrice, &startGas, &feesRes);    
    
    convertUint256BE(G_swap_validated.fee, G_swap_validated.fee_length, &feesValidatedRes);
    

    if (gt256(&feesRes, &feesValidatedRes)){
        PRINTF("Fee does not match \n ");
        return false;
    }
    return true;
}

void swap_finalize_exchange_sign_transaction(bool is_success) {
    PRINTF("Returning to Exchange with status %d\n", is_success);
    *G_swap_sign_return_value_address = is_success ? 1 : 0;
    reset_app_context();
    os_lib_end();
}
#endif // HAVE_SWAP