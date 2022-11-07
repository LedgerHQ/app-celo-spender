#ifdef HAVE_NBGL
#include "globals.h"
#include "ui_common.h"
#include "nbgl_use_case.h"
#include "bolos_target.h"

static void rejectCallback(void) {
    io_seproxyhal_touch_data_cancel();
}

static void approveCallback(void) {
    io_seproxyhal_touch_data_ok();
}

static void confirmationCallback(bool confirm) {
    if (confirm) {
        nbgl_useCaseStatus("SELECTOR\nAPPROVED", true, ui_idle);
        approveCallback();
    }
    else {
        nbgl_useCaseStatus("Selector rejected", false, ui_idle);
        rejectCallback();
    }
}

static void continueCallback(void) {
    tagValueList.pairs = tagValuePair;
    tagValueList.smallCaseForValue = false;

    infoLongPress.text = "Approve selector";
    infoLongPress.icon = &C_celo_32px;
    infoLongPress.longPressText = "Approve";
    infoLongPress.longPressToken = 0;
    infoLongPress.tuneId = TUNE_TAP_CASUAL;

    nbgl_useCaseStaticReview(&tagValueList, &infoLongPress, "Cancel", confirmationCallback);
}

void ui_confirm_selector_flow(void) {
    tagValuePair[0].item = "Selector";
    tagValuePair[0].value = (char*)strings.tmp.tmp;

    tagValueList.nbPairs = 1;

    nbgl_useCaseReviewStart(&C_celo_32px, "Verify selector", "", "Cancel", continueCallback, rejectCallback);
}

void ui_confirm_parameter_flow(void) {
    tagValuePair[0].item = "Parameter";
    tagValuePair[0].value = (char*)strings.tmp.tmp;

    tagValueList.nbPairs = 1;

    nbgl_useCaseReviewStart(&C_celo_32px, "Verify", strings.tmp.tmp2, "Cancel", continueCallback, rejectCallback);
}

#endif // HAVE_NBGL
