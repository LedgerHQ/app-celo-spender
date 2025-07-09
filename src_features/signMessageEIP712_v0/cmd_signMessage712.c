#include <stdint.h>

#include "sw.h"
#include "constants.h"
#include "globals.h"
#include "celo.h"
#include "utils.h"
#include "ui_common.h"
#include "io.h"

// #include "common_utils.h"
// #include "common_ui.h"
// #include "common_712.h"

uint16_t handleSignEIP712Message_v0(uint8_t p1,
                                    const uint8_t *workBuffer,
                                    uint8_t dataLength,
                                    unsigned int *flags) {
    if (p1 != 0x00) {
        return io_send_sw(APDU_RESPONSE_INVALID_P1_P2);
    }
    if (appState != APP_STATE_IDLE) {
        reset_app_context();
    }

    if (!N_storage.blind_signing) {
        ui_error_blind_signing();
        return io_send_sw(APDU_RESPONSE_INVALID_DATA);
    }
    int offset =
        parse_bip32_path(&tmpCtx.messageSigningContext.derivationPath, workBuffer, dataLength);
    if (offset < 0) {
        return io_send_sw(APDU_RESPONSE_INVALID_DATA);
    }
    workBuffer += offset;
    dataLength -= offset;

    if ((workBuffer == NULL) || (dataLength < (KECCAK256_HASH_BYTESIZE * 2))) {
        return io_send_sw(APDU_RESPONSE_INVALID_DATA);
    }
    memmove(tmpCtx.messageSigningContext712.domainHash, workBuffer, KECCAK256_HASH_BYTESIZE);
    memmove(tmpCtx.messageSigningContext712.messageHash,
            workBuffer + KECCAK256_HASH_BYTESIZE,
            KECCAK256_HASH_BYTESIZE);

    ui_sign_712_v0();

    // *flags |= IO_ASYNCH_REPLY;
    return 0;
}
