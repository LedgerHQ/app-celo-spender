#ifdef HAVE_SWAP
#include <string.h>
#include "swap_lib_calls.h"
#include "bip32.h"
#include "cx_errors.h"
#include "crypto_helpers.h"
#include "globals.h"
#include "ethUtils.h"
#include "utils.h"

void swap_handle_check_address(check_address_parameters_t *params) {
    PRINTF("Inside Celo swap_handle_check_address\n");
    params->result = 0;
    //return;
    if (params->address_to_check == 0) {
        PRINTF("Address to check == 0\n");
        //return SW_ERROR_IN_DATA;
        return;
    }

    char address[ADDRESS_LENGTH];
    uint8_t raw_pubkey[65];
    bip32Path_t bip32;
    bip32.len = params->address_parameters[0];
    if (bip32_path_read(params->address_parameters + 1,
                        params->address_parameters_length,
                        bip32.path,
                        bip32.len) == false) {
        PRINTF("Invalid path\n");
        //return SW_ERROR_IN_DATA;
        return;
    }
    CX_THROW(bip32_derive_get_pubkey_256(CX_CURVE_256K1,
                                         bip32.path,
                                         bip32.len,
                                         raw_pubkey,
                                         NULL,
                                         CX_SHA512));
    uint8_t hashAddress[32];
    cx_sha3_t sha3Context;
    CX_THROW(cx_keccak_init_no_throw(&sha3Context, 256));
    CX_THROW(cx_hash_no_throw((cx_hash_t *) &sha3Context, CX_LAST, raw_pubkey + 1, 64, hashAddress, 32));
    getEthAddressStringFromBinary(hashAddress + 12, address, CHAIN_ID, &sha3Context);
    address[ADDRESS_LENGTH - 1] = '\0';

    // PRINTF("Addresses derived: %s\n", address);
    uint8_t offset_0x = 0;
    if (memcmp(params->address_to_check, "0x", 2) == 0) {
        PRINTF("Addresses add shift\n");
        offset_0x = 2;
    }
    if (_strcasecmp(address, params->address_to_check + offset_0x) != 0) {
        PRINTF("Addresses don't match\n");
    } else {
        PRINTF("Addresses match\n");
        params->result = 1;
    }
}
#endif  // HAVE_SWAP