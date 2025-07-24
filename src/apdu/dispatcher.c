#include <stdint.h>

#include "dispatcher.h"
#include "sw.h"
#include "io.h"
#include "globals.h"
#include "manage_asset_info.h"
#include "handlers.h"
#include "commands_712.h"
#include "sign.h"

#include "constants.h"
#include "cmd_tx_info.h"
#include "cmd_field.h"

/**
 * Dispatch structured APDU command to handler
 */
int apdu_dispatcher(const command_t *cmd) {
    // uint32_t *flags = 0;
    // uint16_t sw;

    if (cmd == NULL) {
        io_send_sw(SW_ERROR_IN_DATA);
        return -1;
    }

#ifndef HAVE_WALLET_ID_SDK
    // Handle the GET_WALLET_ID command
    if ((cmd->cla == COMMON_CLA) && (cmd->ins == COMMON_INS_GET_WALLET_ID)) {
        return handler_get_wallet_id(cmd);
    }
#endif

    // Check the CLA byte
    if (cmd->cla != CLA) {
        io_send_sw(SW_CLA_NOT_SUPPORTED);
        return -1;
    }

    // Handle different INS commands
    switch (cmd->ins) {
        case INS_GET_PUBLIC_KEY:
            memset(tmpCtx.transactionContext.assetSet, 0, MAX_ASSETS);
            return handler_get_public_key(cmd);

        case INS_PROVIDE_ERC20_TOKEN_INFORMATION:
            return handler_provide_erc20_token_information(cmd);

        case INS_SIGN:
            return handler_sign(cmd);

        case INS_GET_APP_CONFIGURATION:
            return handler_get_app_configuration(cmd);

        case INS_SIGN_PERSONAL_MESSAGE:
            memset(tmpCtx.transactionContext.assetSet, 0, MAX_ASSETS);
            return handler_sign_personal_message(cmd);

        case INS_GET_APP_TYPE:
            return handler_get_app_type(cmd);

        case INS_SIGN_EIP_712_MESSAGE:
            switch (cmd->p2) {
                case P2_EIP712_LEGACY_IMPLEM:
                    forget_known_assets();
                    return handleSignEIP712Message_v0(cmd->p1, cmd->data, cmd->lc);
                case P2_EIP712_FULL_IMPLEM:
                    return handle_eip712_sign(cmd->data, cmd->lc);
                default:
                    return io_send_sw(APDU_RESPONSE_INVALID_P1_P2);
            }
        case INS_EIP712_STRUCT_DEF:
            return handle_eip712_struct_def(cmd->p2, cmd->data, cmd->lc);

        case INS_EIP712_STRUCT_IMPL:
            return handle_eip712_struct_impl(cmd->p1, cmd->p2, cmd->data, cmd->lc);

        case INS_EIP712_FILTERING:
            return handle_eip712_filtering(cmd->p1, cmd->p2, cmd->data, cmd->lc);
        case INS_GTP_FIELD:
            return handle_field(cmd->p1, cmd->p2, cmd->lc, cmd->data);
        case INS_GTP_TRANSACTION_INFO:
            return handle_tx_info(cmd->p1, cmd->p2, cmd->lc, cmd->data);

        default:
            io_send_sw(SW_INS_NOT_SUPPORTED);
            return -1;
    }
}