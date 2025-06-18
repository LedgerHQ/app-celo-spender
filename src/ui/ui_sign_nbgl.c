#ifdef HAVE_NBGL
#include "globals.h"
#include "ui_common.h"
#include "nbgl_use_case.h"
#include "bolos_target.h"
#include "icons.h"
#include "validate.h"

// static void sign_cancel(void) {
//     io_seproxyhal_touch_signMessage_cancel();
// }

static void sign_msg_confirmation(bool confirm) {
    crypto_sign_message(confirm);
    if (confirm) {
        nbgl_useCaseReviewStatus(STATUS_TYPE_MESSAGE_SIGNED, ui_idle);
    } else {
        nbgl_useCaseReviewStatus(STATUS_TYPE_MESSAGE_REJECTED, ui_idle);
    }
}

void ui_display_sign_flow(void) {
    tagValuePair[0].item = "Message hash";
    tagValuePair[0].value = (char *) strings.common.fullAddress;

    tagValueList.nbPairs = 1;
    tagValueList.pairs = tagValuePair;
    tagValueList.smallCaseForValue = false;

    nbgl_useCaseReview(TYPE_MESSAGE,
                       &tagValueList,
                       &ICON_APP_CELO,
                       "Review message",
                       NULL,
                       "Sign message",
                       sign_msg_confirmation);
}
#endif  // HAVE_NBGL
