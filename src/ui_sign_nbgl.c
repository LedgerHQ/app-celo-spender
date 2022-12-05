#ifdef HAVE_NBGL
#include "globals.h"
#include "ui_common.h"
#include "nbgl_use_case.h"
#include "bolos_target.h"

static void signConfirmation(bool confirm) {
    if (confirm) {
        nbgl_useCaseStatus("MESSAGE\nAPPROVED", true, ui_idle);
        io_seproxyhal_touch_signMessage_ok();
    }
    else {
        nbgl_useCaseStatus("Message rejected", false, ui_idle);
        io_seproxyhal_touch_signMessage_cancel();
    }
}

void ui_display_sign_flow(void) {
    tagValuePair[0].item = "Message hash";
    tagValuePair[0].value = (char*)strings.common.fullAddress;

    tagValueList.nbPairs = 1;
    tagValueList.pairs = tagValuePair;
    tagValueList.smallCaseForValue = false;

    infoLongPress.text = "Sign message";
    infoLongPress.icon = &C_celo_64px;
    infoLongPress.longPressText = "Approve";
    infoLongPress.longPressToken = 0;
    infoLongPress.tuneId = TUNE_TAP_CASUAL;

    nbgl_useCaseStaticReview(&tagValueList, &infoLongPress, "Cancel", signConfirmation);
}
#endif // HAVE_NBGL
