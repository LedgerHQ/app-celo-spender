#include "get_public_key.h"

#include "io.h"
#include "sw.h"
#include "os.h"
#include "globals.h"
#include "constants.h"
#include "celo.h"
#include "utils.h"
#include "ethUtils.h"
#include "ui_common.h"
#include "send_response.h"

/**
 * Handle Get Public Key command
 *
 * @param p1 The first parameter
 * @param p2 The second parameter
 * @param dataBuffer The data buffer
 * @param dataLength The length of the data buffer
 */
int handleGetPublicKey(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength) {
    UNUSED(dataLength);
    uint8_t privateKeyData[64];
    bip32Path_t derivationPath;
    cx_ecfp_private_key_t privateKey;

    reset_app_context();
    if ((p1 != P1_CONFIRM) && (p1 != P1_NON_CONFIRM)) {
        return io_send_sw(SW_WRONG_P1_OR_P2);
    }
    if ((p2 != P2_CHAINCODE) && (p2 != P2_NO_CHAINCODE)) {
        return io_send_sw(SW_WRONG_P1_OR_P2);
    }

    if (parse_bip32_path(&derivationPath, dataBuffer, dataLength)) {
        PRINTF("Invalid path\n");
        return io_send_sw(SW_ERROR_IN_DATA);
    }

    tmpCtx.publicKeyContext.getChaincode = (p2 == P2_CHAINCODE);
    cx_err_t error = os_derive_bip32_no_throw(
        CX_CURVE_256K1,
        derivationPath.path,
        derivationPath.len,
        privateKeyData,
        (tmpCtx.publicKeyContext.getChaincode ? tmpCtx.publicKeyContext.chainCode : NULL));

    if (error != CX_OK) {
        return io_send_sw(error);
    }
    error = cx_ecfp_init_private_key_no_throw(CX_CURVE_256K1, privateKeyData, 32, &privateKey);
    if (error != CX_OK) {
        return io_send_sw(error);
    }
    error = cx_ecfp_generate_pair_no_throw(CX_CURVE_256K1,
                                           &tmpCtx.publicKeyContext.publicKey,
                                           &privateKey,
                                           1);

    if (error != CX_OK) {
        return io_send_sw(error);
    }

    explicit_bzero(&privateKey, sizeof(privateKey));
    explicit_bzero(privateKeyData, sizeof(privateKeyData));
    getEthAddressStringFromKey(&tmpCtx.publicKeyContext.publicKey,
                               tmpCtx.publicKeyContext.address,
                               CHAIN_ID,
                               &sha3);
#ifndef NO_CONSENT
    if (p1 == P1_NON_CONFIRM)
#endif  // NO_CONSENT
    {
        return helper_send_response_pubkey();
    }
#ifndef NO_CONSENT
    else {
        // prepare for a UI based reply
        snprintf(strings.common.fullAddress,
                 sizeof(strings.common.fullAddress),
                 "0x%.*s",
                 40,
                 tmpCtx.publicKeyContext.address);
        ui_display_address();
        return 0;
    }
#endif  // NO_CONSENT
}
