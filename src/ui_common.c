#include "ui_common.h"

#include "celo.h"
#include "os_io_seproxyhal.h" // for heartbeat
#include "globals.h"
#include "utils.h"

#ifdef HAVE_NBGL
nbgl_layoutTagValue_t tagValuePair[5];
nbgl_layoutTagValueList_t tagValueList;
nbgl_pageInfoLongPress_t infoLongPress;
#endif // HAVE_NBGL

unsigned int io_seproxyhal_touch_data_ok(void) {
    parserStatus_e txResult = USTREAM_FINISHED;
    txResult = continueTx(&txContext);
    switch (txResult) {
    case USTREAM_SUSPENDED:
        break;
    case USTREAM_FINISHED:
        break;
    case USTREAM_PROCESSING:
        io_seproxyhal_send_status(0x9000);
#ifdef HAVE_BAGL
        ui_idle();
#endif // HAVE_BAGL
        break;
    case USTREAM_FAULT:
        reset_app_context();
        io_seproxyhal_send_status(0x6A80);
#ifdef HAVE_BAGL
        ui_idle();
#endif // HAVE_BAGL
        break;
    default:
        PRINTF("Unexpected parser status\n");
        reset_app_context();
        io_seproxyhal_send_status(0x6A80);
#ifdef HAVE_BAGL
        ui_idle();
#endif // HAVE_BAGL
    }

    if (txResult == USTREAM_FINISHED) {
        finalizeParsing(false);
    }

    return 0;
}

unsigned int io_seproxyhal_touch_data_cancel(void) {
    reset_app_context();
    io_seproxyhal_send_status(0x6985);
#ifdef HAVE_BAGL
    // Display back the original UX
    ui_idle();
#endif // HAVE_BAGL
    return 0; // do not redraw the widget
}

unsigned int io_seproxyhal_touch_address_ok(void) {
    uint32_t tx = set_result_get_publicKey();
    G_io_apdu_buffer[tx++] = 0x90;
    G_io_apdu_buffer[tx++] = 0x00;
    reset_app_context();
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
#ifdef HAVE_BAGL
    // Display back the original UX
    ui_idle();
#endif // HAVE_BAGL
    return 0; // do not redraw the widget
}

unsigned int io_seproxyhal_touch_address_cancel(void) {
    G_io_apdu_buffer[0] = 0x69;
    G_io_apdu_buffer[1] = 0x85;
    reset_app_context();
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
#ifdef HAVE_BAGL
    // Display back the original UX
    ui_idle();
#endif // HAVE_BAGL
    return 0; // do not redraw the widget
}

unsigned int io_seproxyhal_touch_tx_ok(void) {
    uint8_t privateKeyData[32];
    uint8_t signature[100];
    cx_ecfp_private_key_t privateKey;
    uint32_t tx = 0;
    uint32_t v = getV(&tmpContent.txContent);
    io_seproxyhal_io_heartbeat();
    os_perso_derive_node_bip32(CX_CURVE_256K1, tmpCtx.transactionContext.derivationPath.path,
                               tmpCtx.transactionContext.derivationPath.len,
                               privateKeyData, NULL);
    cx_ecfp_init_private_key(CX_CURVE_256K1, privateKeyData, 32,
                                 &privateKey);
    explicit_bzero(privateKeyData, sizeof(privateKeyData));
    unsigned int info = 0;
    io_seproxyhal_io_heartbeat();
    cx_ecdsa_sign(&privateKey, CX_RND_RFC6979 | CX_LAST, CX_SHA256,
                  tmpCtx.transactionContext.hash,
                  sizeof(tmpCtx.transactionContext.hash), signature, sizeof(signature), &info);
    explicit_bzero(&privateKey, sizeof(privateKey));
    // Parity is present in the sequence tag in the legacy API
    if (tmpContent.txContent.vLength == 0) {
      // Legacy API
      G_io_apdu_buffer[0] = 27;
    }
    else {
      // New API
      // Note that this is wrong for a large v, but the client can always recover
      G_io_apdu_buffer[0] = (v * 2) + 35;
    }
    if (info & CX_ECCINFO_PARITY_ODD) {
      G_io_apdu_buffer[0]++;
    }
    if (info & CX_ECCINFO_xGTn) {
      G_io_apdu_buffer[0] += 2;
    }
    format_signature_out(signature);
    tx = 65;
    G_io_apdu_buffer[tx++] = 0x90;
    G_io_apdu_buffer[tx++] = 0x00;
    reset_app_context();
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
#ifdef HAVE_BAGL
    // Display back the original UX
    ui_idle();
#endif // HAVE_BAGL
    return 0; // do not redraw the widget
}

unsigned int io_seproxyhal_touch_tx_cancel(void) {
    reset_app_context();
    G_io_apdu_buffer[0] = 0x69;
    G_io_apdu_buffer[1] = 0x85;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
#ifdef HAVE_BAGL
    // Display back the original UX
    ui_idle();
#endif // HAVE_BAGL
    return 0; // do not redraw the widget
}

unsigned int io_seproxyhal_touch_signMessage_ok(void) {
    uint8_t privateKeyData[32];
    uint8_t signature[100];
    cx_ecfp_private_key_t privateKey;
    uint32_t tx = 0;
    io_seproxyhal_io_heartbeat();
    os_perso_derive_node_bip32(
        CX_CURVE_256K1, tmpCtx.messageSigningContext.derivationPath.path,
        tmpCtx.messageSigningContext.derivationPath.len, privateKeyData, NULL);
    io_seproxyhal_io_heartbeat();
    cx_ecfp_init_private_key(CX_CURVE_256K1, privateKeyData, 32, &privateKey);
    explicit_bzero(privateKeyData, sizeof(privateKeyData));
    unsigned int info = 0;
    io_seproxyhal_io_heartbeat();
    cx_ecdsa_sign(&privateKey, CX_RND_RFC6979 | CX_LAST, CX_SHA256,
                  tmpCtx.messageSigningContext.hash,
                  sizeof(tmpCtx.messageSigningContext.hash), signature, sizeof(signature), &info);
    explicit_bzero(&privateKey, sizeof(privateKey));
    G_io_apdu_buffer[0] = 27;
    if (info & CX_ECCINFO_PARITY_ODD) {
      G_io_apdu_buffer[0]++;
    }
    if (info & CX_ECCINFO_xGTn) {
      G_io_apdu_buffer[0] += 2;
    }
    format_signature_out(signature);
    tx = 65;
    G_io_apdu_buffer[tx++] = 0x90;
    G_io_apdu_buffer[tx++] = 0x00;
    reset_app_context();
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
#ifdef HAVE_BAGL
    // Display back the original UX
    ui_idle();
#endif // HAVE_BAGL
    return 0; // do not redraw the widget
}

unsigned int io_seproxyhal_touch_signMessage_cancel(void) {
    reset_app_context();
    G_io_apdu_buffer[0] = 0x69;
    G_io_apdu_buffer[1] = 0x85;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
#ifdef HAVE_BAGL
    // Display back the original UX
    ui_idle();
#endif // HAVE_BAGL
    return 0; // do not redraw the widget
}
