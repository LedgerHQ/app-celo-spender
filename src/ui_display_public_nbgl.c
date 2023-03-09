#ifdef HAVE_NBGL
#include "globals.h"
#include "ui_common.h"
#include "bolos_target.h"

static void address_cancel(void) {
    nbgl_useCaseStatus("Address rejected", false, ui_idle);
    io_seproxyhal_touch_address_cancel();
}

static void address_confirmation(bool confirm) {
    if (confirm) {
        nbgl_useCaseStatus("ADDRESS\nAPPROVED", true, ui_idle);
        io_seproxyhal_touch_address_ok();
    }
    else {
        address_cancel();
    }
}

static void address_display(void) {
    nbgl_useCaseAddressConfirmation(strings.common.fullAddress, address_confirmation);
}

void ui_display_public_flow(void) {
    nbgl_useCaseReviewStart(&C_celo_64px,
                            "Verify Celo\naddress",
                            "",
                            "Cancel",
                            address_display,
                            address_cancel);
}

#endif // HAVE_NBGL
