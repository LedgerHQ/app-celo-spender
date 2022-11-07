#ifdef HAVE_NBGL
#include "globals.h"
#include "ui_common.h"
#include "bolos_target.h"

static void addressConfirmation(bool confirm) {
    if (confirm) {
        nbgl_useCaseStatus("ADDRESS\nAPPROVED", true, ui_idle);
        io_seproxyhal_touch_address_ok();
    }
    else {
        nbgl_useCaseStatus("Address rejected", false, ui_idle);
        io_seproxyhal_touch_address_cancel();
    }
}

void ui_display_public_flow(void) {
    nbgl_useCaseAddressConfirmation(strings.common.fullAddress, addressConfirmation);
}

#endif // HAVE_NBGL
