#ifdef HAVE_NBGL
#include "globals.h"
#include "ui_common.h"
#include "bolos_target.h"

static void address_cancel(void) {
    nbgl_useCaseReviewStatus(STATUS_TYPE_ADDRESS_REJECTED, ui_idle);
    io_seproxyhal_touch_address_cancel();
}

static void address_confirmation(bool confirm) {
    if (confirm) {
        nbgl_useCaseReviewStatus(STATUS_TYPE_ADDRESS_VERIFIED, ui_idle);
        io_seproxyhal_touch_address_ok();
    }
    else {
        address_cancel();
    }
}

void ui_display_public_flow(void) {
    nbgl_useCaseAddressReview(strings.common.fullAddress,
                              NULL,
                              &ICON_APP_CELO,
                              "Verify Celo\naddress",
                              NULL,
                              address_confirmation);
}

#endif // HAVE_NBGL
