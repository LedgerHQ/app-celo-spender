#ifdef HAVE_NBGL
#include "globals.h"
#include "ui_common.h"
#include "nbgl_use_case.h"
#include "bolos_target.h"

static void sign_cancel(void) {
    nbgl_useCaseReviewStatus(STATUS_TYPE_MESSAGE_REJECTED, ui_idle);
    io_seproxyhal_touch_signMessage_cancel();
}

static void sign_confirmation(bool confirm) {
    if (confirm) {
        nbgl_useCaseReviewStatus(STATUS_TYPE_MESSAGE_SIGNED, ui_idle);
        io_seproxyhal_touch_signMessage_ok();
    }
    else {
        sign_cancel();
    }
}

void ui_display_sign_flow(void) {
    tagValuePair[0].item = "Message hash";
    tagValuePair[0].value = (char*)strings.common.fullAddress;

    tagValueList.nbPairs = 1;
    tagValueList.pairs = tagValuePair;
    tagValueList.smallCaseForValue = false;

    nbgl_useCaseReview(TYPE_MESSAGE, &tagValueList, &C_celo_64px, "Review message", NULL, "Sign message", sign_confirmation);
}
#endif // HAVE_NBGL
