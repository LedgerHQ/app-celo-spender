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
        nbgl_useCaseReviewStatus(STATUS_TYPE_OPERATION_SIGNED, ui_idle);
        approveCallback();
    }
    else {
        nbgl_useCaseReviewStatus(STATUS_TYPE_OPERATION_REJECTED, ui_idle);
        rejectCallback();
    }
}

void ui_confirm_selector_flow(void) {
    tagValuePair[0].item = "Selector";
    tagValuePair[0].value = (char*)strings.tmp.tmp;

    tagValueList.nbMaxLinesForValue = 0;
    tagValueList.nbPairs = 1;
    tagValueList.pairs = tagValuePair;

    nbgl_useCaseReview(TYPE_TRANSACTION, &tagValueList, &C_celo_64px, "Verify selector", NULL, "Confirm selector", confirmationCallback);
}

void ui_confirm_parameter_flow(void) {
    tagValuePair[0].item = "Parameter";
    tagValuePair[0].value = (char*)strings.tmp.tmp;

    tagValueList.nbMaxLinesForValue = 0;
    tagValueList.nbPairs = 1;
    tagValueList.pairs = tagValuePair;

    nbgl_useCaseReview(TYPE_TRANSACTION, &tagValueList, &C_celo_64px, "Verify", NULL, "Confirm", confirmationCallback);
}

#endif // HAVE_NBGL
