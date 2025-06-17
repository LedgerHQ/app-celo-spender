#ifdef HAVE_NBGL
#include "globals.h"
#include "ui_common.h"
#include "bolos_target.h"
#include "icons.h"
#include "send_response.h"
#include "validate.h"


static void address_confirmation(bool confirm) {
    validate_pubkey(confirm);
    if (confirm) {
        nbgl_useCaseReviewStatus(STATUS_TYPE_ADDRESS_VERIFIED, ui_idle);
    }
    else {
        nbgl_useCaseReviewStatus(STATUS_TYPE_ADDRESS_REJECTED, ui_idle);
    }
}

void ui_display_address(void) {
    nbgl_useCaseAddressReview(tmpCtx.publicKeyContext.address,
                              NULL,
                              &ICON_APP_CELO,
                              "Verify Celo\naddress",
                              NULL,
                              address_confirmation);
}

#endif // HAVE_NBGL
