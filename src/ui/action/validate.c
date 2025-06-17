#include "io.h"
#include "sw.h"
#include "send_response.h"
#include "validate.h"

void validate_pubkey(bool confirm) {
    if (confirm) {
        helper_send_response_pubkey();
    }
    else {
        io_send_sw(SW_DENY);
    }
}