#include "io.h"
#include "sw.h"
#include "send_response.h"
#include "validate.h"
#include "cx.h"
#include "os_seed.h"
#include "globals.h"

void validate_pubkey(bool confirm) {
    if (confirm) {
        helper_send_response_pubkey();
    } else {
        io_send_sw(SW_DENY);
    }
}

int crypto_sign_message(bool confirm) {
    if (!confirm) {
        return io_send_sw(SW_DENY);
    }
    uint8_t privateKeyData[64];
    uint8_t signature[100];
    cx_ecfp_private_key_t privateKey;
    // io_seproxyhal_io_heartbeat();
    cx_err_t err = os_derive_bip32_no_throw(CX_CURVE_256K1,
                                            tmpCtx.messageSigningContext.derivationPath.path,
                                            tmpCtx.messageSigningContext.derivationPath.len,
                                            privateKeyData,
                                            NULL);
    if (err != CX_OK) {
        return io_send_sw(err);
    }
    // io_seproxyhal_io_heartbeat();
    err = cx_ecfp_init_private_key_no_throw(CX_CURVE_256K1, privateKeyData, 32, &privateKey);
    if (err != CX_OK) {
        return io_send_sw(err);
    }
    explicit_bzero(privateKeyData, sizeof(privateKeyData));
    unsigned int info = 0;
    size_t sig_len = sizeof(signature);
    // io_seproxyhal_io_heartbeat();
    err = cx_ecdsa_sign_no_throw(&privateKey,
                                 CX_RND_RFC6979 | CX_LAST,
                                 CX_SHA256,
                                 tmpCtx.messageSigningContext.hash,
                                 sizeof(tmpCtx.messageSigningContext.hash),
                                 signature,
                                 &sig_len,
                                 &info);
    if (err != CX_OK) {
        return io_send_sw(err);
    }
    explicit_bzero(&privateKey, sizeof(privateKey));
    return helper_send_response_signature(signature, info);
}