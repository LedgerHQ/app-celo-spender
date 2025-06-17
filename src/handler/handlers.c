#include <string.h>
#include "io.h"

#include "handlers.h"
#include "sw.h"
#include "globals.h"
#include "celo.h"
#include "ui_common.h"
#include "constants.h"

#include "get_public_key.h"
#include "provide_erc20_token_information.h"
#include "sign.h"
#include "get_app_info.h"
#include "sign_personal_message.h"


#ifndef HAVE_WALLET_ID_SDK
extern void handleGetWalletId(volatile unsigned int *tx);
#endif

// Adapter functions for the new architecture

int handler_get_public_key(const command_t *cmd) {    
    BEGIN_TRY {
        TRY {
            return handleGetPublicKey(cmd->p1, cmd->p2, (uint8_t*)cmd->data, cmd->lc);
        }
        CATCH_OTHER(e) {
            reset_app_context();
            return io_send_sw(e);
        }
        FINALLY {
        }
    }
    END_TRY;
    
}

int handler_provide_erc20_token_information(const command_t *cmd) {
    volatile unsigned int flags = 0;
    volatile unsigned int tx = 0;
    
    BEGIN_TRY {
        TRY {
            handleProvideErc20TokenInformation(cmd->p1, cmd->p2, (uint8_t*)cmd->data, cmd->lc, &flags, &tx);
            return io_send_sw(SW_OK);
        }
        CATCH_OTHER(e) {
            reset_app_context();
            return io_send_sw(e);
        }
        FINALLY {
        }
    }
    END_TRY;
    
    return 0;
}

int handler_sign(const command_t *cmd) {
    volatile unsigned int flags = 0;
    volatile unsigned int tx = 0;
    
    BEGIN_TRY {
        TRY {
            handleSign(cmd->p1, cmd->p2, cmd->data, cmd->lc, &flags, &tx);
            
            if (!(flags & IO_ASYNCH_REPLY)) {
                return io_send_response_pointer(G_io_apdu_buffer, tx, SW_OK);
            }
        }
        CATCH_OTHER(e) {
            reset_app_context();
            return io_send_sw(e);
        }
        FINALLY {
        }
    }
    END_TRY;
    
    return 0;
}

int handler_get_app_configuration(const command_t *cmd) {
    volatile unsigned int flags = 0;
    volatile unsigned int tx = 0;
    
    BEGIN_TRY {
        TRY {
            handleGetAppConfiguration(cmd->p1, cmd->p2, (uint8_t*)cmd->data, cmd->lc, &flags, &tx);
            return io_send_response_pointer(G_io_apdu_buffer, tx, SW_OK);
        }
        CATCH_OTHER(e) {
            reset_app_context();
            return io_send_sw(e);
        }
        FINALLY {
        }
    }
    END_TRY;
    
    return 0;
}

int handler_sign_personal_message(const command_t *cmd) {
    volatile unsigned int flags = 0;
    volatile unsigned int tx = 0;
    
    BEGIN_TRY {
        TRY {
            handleSignPersonalMessage(cmd->p1, cmd->p2, (uint8_t*)cmd->data, cmd->lc, &flags, &tx);
            
            if (!(flags & IO_ASYNCH_REPLY)) {
                return io_send_response_pointer(G_io_apdu_buffer, tx, SW_OK);
            }
        }
        CATCH_OTHER(e) {
            reset_app_context();
            return io_send_sw(e);
        }
        FINALLY {
        }
    }
    END_TRY;
    
    return 0;
}

int handler_get_app_type(const command_t *cmd) {
    volatile unsigned int flags = 0;
    volatile unsigned int tx = 0;
    
    BEGIN_TRY {
        TRY {
            handleGetAppType(cmd->p1, cmd->p2, (uint8_t*)cmd->data, cmd->lc, &flags, &tx);
            return io_send_response_pointer(G_io_apdu_buffer, tx, SW_OK);
        }
        CATCH_OTHER(e) {
            reset_app_context();
            return io_send_sw(e);
        }
        FINALLY {
        }
    }
    END_TRY;
    
    return 0;
}

#ifndef HAVE_WALLET_ID_SDK
int handler_get_wallet_id(const command_t *cmd) {
    (void)cmd; // Unused parameter
    volatile unsigned int tx = 0;
    
    BEGIN_TRY {
        TRY {
            handleGetWalletId(&tx);
            return io_send_response_pointer(G_io_apdu_buffer, tx, SW_OK);
        }
        CATCH_OTHER(e) {
            reset_app_context();
            return io_send_sw(e);
        }
        FINALLY {
        }
    }
    END_TRY;
    
    return 0;
}
#endif 