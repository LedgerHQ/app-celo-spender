#include "sign.h"

#include "io.h"
#include "sw.h"
#include "os.h"
#include "cx.h"
#include "ui_common.h"
#include "globals.h"
#include "constants.h"
#include "celo.h"
#include "utils.h"
#include "ethUtils.h"

/**
 * Handles the signing of a transaction.
 *
 * @param p1 The first parameter of the APDU command.
 * @param p2 The second parameter of the APDU command.
 * @param workBuffer The buffer containing the transaction data.
 * @param dataLength The length of the transaction data.
 * @param flags The flags variable.
 * @param tx The transaction variable.
 */
int handleSign(uint8_t p1,
               uint8_t p2,
               const uint8_t *workBuffer,
               uint16_t dataLength,
               volatile unsigned int *flags) {
    parserStatus_e txResult;
    cx_err_t err;
    if (p1 == P1_FIRST) {
        if (appState != APP_STATE_IDLE) {
            reset_app_context();
        }

        if (parse_bip32_path(&tmpCtx.transactionContext.derivationPath, workBuffer, dataLength)) {
            PRINTF("Invalid path\n");
            return io_send_sw(SW_ERROR_IN_DATA);
        }
        workBuffer += 1 + tmpCtx.transactionContext.derivationPath.len * sizeof(uint32_t);
        dataLength -= 1 + tmpCtx.transactionContext.derivationPath.len * sizeof(uint32_t);

        appState = APP_STATE_SIGNING_TX;
        dataPresent = false;
        provisionType = PROVISION_NONE;
        initTx(&txContext, &sha3, &tmpContent.txContent, customProcessor, NULL);
        // Extract and validate the transaction type
        uint8_t txType = *workBuffer;
        if (txType == EIP1559 || txType == CIP64) {
            // Initialize the SHA3 hashing with the transaction type
            CX_THROW(cx_hash_no_throw((cx_hash_t *) &sha3, 0, workBuffer, 1, NULL, 0));
            // Save the transaction type
            txContext.txType = txType;
            workBuffer++;
            dataLength--;
        } else {
            return io_send_sw(SW_TX_TYPE_NOT_SUPPORTED);
        }
    } else if (p1 != P1_MORE) {
        return io_send_sw(SW_WRONG_P1_OR_P2);
    }
    if (p2 != 0) {
        return io_send_sw(SW_WRONG_P1_OR_P2);
    }
    if ((p1 == P1_MORE) && (appState != APP_STATE_SIGNING_TX)) {
        PRINTF("Signature not initialized\n");
        return io_send_sw(SW_INITIALIZATION_ERROR);
    }
    if (txContext.currentField == RLP_NONE) {
        PRINTF("Parser not initialized\n");
        return io_send_sw(SW_INITIALIZATION_ERROR);
    }
    txResult = processTx(&txContext, workBuffer, dataLength);
    switch (txResult) {
        case USTREAM_SUSPENDED:
            break;
        case USTREAM_FINISHED:
            break;
        case USTREAM_PROCESSING:
            return io_send_sw(SW_OK);
        case USTREAM_FAULT:
            return io_send_sw(SW_ERROR_IN_DATA);
        default:
            PRINTF("Unexpected parser status\n");
            return io_send_sw(SW_ERROR_IN_DATA);
    }

    *flags |= IO_ASYNCH_REPLY;

    if (txResult == USTREAM_FINISHED) {
        finalizeParsing(true);
    }
}
