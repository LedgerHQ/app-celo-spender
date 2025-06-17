/*******************************************************************************
*   Ledger Ethereum App
*   (c) 2016-2019 Ledger
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "os.h"
#include "cx.h"
#include "ethUstream.h"
#include "ethUtils.h"
#include "uint256.h"
#include "types.h"
#include "celo.h"

#include "os_io_seproxyhal.h"

#include "globals.h"
#include "utils.h"
#include "ui_common.h"

// New architecture includes
#include "io.h"
#include "sw.h"
#include "dispatcher.h"

// Latest includes 
#include "constants.h"

// Contexts
dataContext_t dataContext;
tmpCtx_t tmpCtx;
txContext_t txContext;
tmpContent_t tmpContent;
cx_sha3_t sha3;

// Volatile variables
volatile uint8_t dataAllowed;
volatile uint8_t contractDetails;
volatile bool dataPresent;
volatile provision_type_t provisionType;

// Strings
strings_t strings;

// Internal storage
const internalStorage_t N_storage_real;

/**
 * Handle APDU command received and send back APDU response using handlers.
 * This follows the new boilerplate architecture pattern.
 */
void app_main() {
    // Length of APDU command received in G_io_apdu_buffer
    int input_len = 0;
    // Structured APDU command
    command_t cmd;


    io_init();

    ui_idle();

    // Reset context
    reset_app_context();
    tmpCtx.transactionContext.currentTokenIndex = 0;


    // Initialize the NVM data if required
    if (N_storage.initialized != 0x01) {
        internalStorage_t storage;
        storage.dataAllowed = 0x00;
        storage.contractDetails = 0x00;
        storage.initialized = 0x01;
        nvm_write(&N_storage, (void*)&storage, sizeof(internalStorage_t));
    }
    
    dataAllowed = N_storage.dataAllowed;
    contractDetails = N_storage.contractDetails;

    for (;;) {
        // Receive command bytes in G_io_apdu_buffer
        if ((input_len = io_recv_command()) < 0) {
            PRINTF("=> io_recv_command failure\n");
            return;
        }

        // Parse APDU command from G_io_apdu_buffer
        if (!apdu_parser(&cmd, G_io_apdu_buffer, input_len)) {
            PRINTF("=> /!\\ BAD LENGTH: %.*H\n", input_len, G_io_apdu_buffer);
            io_send_sw(SW_WRONG_DATA_LENGTH);
            continue;
        }

        PRINTF("=> CLA=%02X | INS=%02X | P1=%02X | P2=%02X | Lc=%02X | CData=%.*H\n",
               cmd.cla,
               cmd.ins,
               cmd.p1,
               cmd.p2,
               cmd.lc,
               cmd.lc,
               cmd.data);

        // Dispatch structured APDU command to handler
        if (apdu_dispatcher(&cmd) < 0) {
            PRINTF("=> apdu_dispatcher failure\n");
            return;
        }
    }
}
