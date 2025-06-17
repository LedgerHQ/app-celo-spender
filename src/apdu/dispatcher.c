#include "dispatcher.h"
#include "sw.h"
#include "io.h"
#include "globals.h"
#include "celo.h"
#include "handlers.h"

#include "constants.h"


/**
 * Dispatch structured APDU command to handler
 */
int apdu_dispatcher(const command_t *cmd) {
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
            memset(tmpCtx.transactionContext.tokenSet, 0, MAX_TOKEN);
            return handler_get_public_key(cmd);

        case INS_PROVIDE_ERC20_TOKEN_INFORMATION:
            return handler_provide_erc20_token_information(cmd);

        case INS_SIGN:
            return handler_sign(cmd);

        case INS_GET_APP_CONFIGURATION:
            return handler_get_app_configuration(cmd);

        case INS_SIGN_PERSONAL_MESSAGE:
            memset(tmpCtx.transactionContext.tokenSet, 0, MAX_TOKEN);
            return handler_sign_personal_message(cmd);

        case INS_GET_APP_TYPE:
            return handler_get_app_type(cmd);

        default:
            io_send_sw(SW_INS_NOT_SUPPORTED);
            return -1;
    }
} 