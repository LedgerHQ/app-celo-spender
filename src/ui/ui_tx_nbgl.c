#ifdef HAVE_NBGL
#include "globals.h"
#include "ui_common.h"
#include "nbgl_use_case.h"
#include "bolos_target.h"
#include "icons.h"

static void rejectCallback(void) {
    io_seproxyhal_touch_tx_cancel();
}

static void approveCallback(void) {
    io_seproxyhal_touch_tx_ok();
}

static void confirmationCallback(bool confirm) {
    if (confirm) {
        nbgl_useCaseReviewStatus(STATUS_TYPE_TRANSACTION_SIGNED, ui_idle);
        approveCallback();
    }
    else {
        nbgl_useCaseReviewStatus(STATUS_TYPE_TRANSACTION_REJECTED, ui_idle);
        rejectCallback();
    }
}

static void fill_data_tx(void) {
    tagValuePair[0].item = "Amount";
    tagValuePair[0].value = (char*)strings.common.fullAmount;

    tagValuePair[1].item = "Address";
    tagValuePair[1].value = (char*)strings.common.fullAddress;

    tagValuePair[2].item = "Max Fees";
    tagValuePair[2].value = (char*)strings.common.maxFee;

    tagValueList.nbPairs = 3;
    tagValueList.pairs = tagValuePair;
}

static void fill_gateway_tx(void) {
    tagValuePair[0].item = "Amount";
    tagValuePair[0].value = (char*)strings.common.fullAmount;

    tagValuePair[1].item = "Address";
    tagValuePair[1].value = (char*)strings.common.fullAddress;

    tagValuePair[2].item = "Max Fees";
    tagValuePair[2].value = (char*)strings.common.maxFee;

    tagValuePair[3].item = "Gateway Fee";
    tagValuePair[3].value = (char*)strings.common.gatewayFee;

    tagValuePair[4].item = "Gateway Address";
    tagValuePair[4].value = (char*)strings.common.fullGatewayAddress;

    tagValueList.nbPairs = 5;
    tagValueList.pairs = tagValuePair;
}

static void fill_lock_relock(void) {
    tagValuePair[0].item = "Type";
    tagValuePair[0].value = (char*)strings.common.stakingType;

    tagValuePair[1].item = "Amount";
    tagValuePair[1].value = (char*)strings.common.fullAmount;

    tagValuePair[2].item = "Max Fees";
    tagValuePair[2].value = (char*)strings.common.maxFee;

    tagValueList.nbPairs = 3;
    tagValueList.pairs = tagValuePair;
}

static void fill_withdraw_or_create_account(void) {
    tagValuePair[0].item = "Type";
    tagValuePair[0].value = (char*)strings.common.stakingType;

    tagValuePair[1].item = "Max Fees";
    tagValuePair[1].value = (char*)strings.common.maxFee;

    tagValueList.nbPairs = 2;
    tagValueList.pairs = tagValuePair;
}

static void fill_activate(void) {
    tagValuePair[0].item = "Type";
    tagValuePair[0].value = (char*)strings.common.stakingType;

    tagValuePair[1].item = "Validator";
    tagValuePair[1].value = (char*)strings.common.fullAddress;

    tagValuePair[2].item = "Max Fees";
    tagValuePair[2].value = (char*)strings.common.maxFee;

    tagValueList.nbPairs = 3;
    tagValueList.pairs = tagValuePair;
}

static void fill_vote_revoke(void) {
    tagValuePair[0].item = "Type";
    tagValuePair[0].value = (char*)strings.common.stakingType;

    tagValuePair[1].item = "Amount";
    tagValuePair[1].value = (char*)strings.common.fullAmount;

    tagValuePair[2].item = "Validator";
    tagValuePair[2].value = (char*)strings.common.fullAddress;

    tagValuePair[3].item = "Max Fees";
    tagValuePair[3].value = (char*)strings.common.maxFee;

    tagValueList.nbPairs = 4;
    tagValueList.pairs = tagValuePair;
}

void ui_approval_celo_tx_flow(void) {
    fill_data_tx();
    nbgl_useCaseReview(TYPE_TRANSACTION, &tagValueList, &ICON_APP_CELO, "Review transaction", NULL, "Approve Transaction", confirmationCallback);
}

void ui_approval_celo_data_warning_tx_flow(void) {
    fill_data_tx();
    
    // For transactions with data present, we can use nbgl_useCaseReview with appropriate warning
    // The warning will be handled by the NBGL framework automatically based on transaction type
    nbgl_useCaseReview(TYPE_TRANSACTION, &tagValueList, &ICON_APP_WARNING, "WARNING", "Data present in transaction", "Approve Transaction", confirmationCallback);
}

void ui_approval_celo_gateway_tx_flow(void) {
    fill_gateway_tx();
    nbgl_useCaseReview(TYPE_TRANSACTION, &tagValueList, &ICON_APP_CELO, "Review transaction", NULL, "Approve Transaction", confirmationCallback);
}

void ui_approval_celo_data_warning_gateway_tx_flow(void) {
    fill_gateway_tx();
    
    // For gateway transactions with data present
    nbgl_useCaseReview(TYPE_TRANSACTION, &tagValueList, &ICON_APP_WARNING, "WARNING", "Data present in transaction", "Approve Transaction", confirmationCallback);
}

void ui_approval_celo_lock_unlock_flow(void) {
    fill_lock_relock();
    nbgl_useCaseReview(TYPE_TRANSACTION, &tagValueList, &ICON_APP_CELO, "Review transaction", NULL, "Approve Transaction", confirmationCallback);
}

void ui_approval_celo_relock_flow(void) {
    fill_lock_relock();
    nbgl_useCaseReview(TYPE_TRANSACTION, &tagValueList, &ICON_APP_CELO, "Review transaction", NULL, "Approve Transaction", confirmationCallback);
}

void ui_approval_celo_withdraw_flow(void) {
    fill_withdraw_or_create_account();
    nbgl_useCaseReview(TYPE_TRANSACTION, &tagValueList, &ICON_APP_CELO, "Review transaction", NULL, "Approve Transaction", confirmationCallback);
}

void ui_approval_celo_create_account_flow(void) {
    fill_withdraw_or_create_account();
    nbgl_useCaseReview(TYPE_TRANSACTION, &tagValueList, &ICON_APP_CELO, "Review transaction", NULL, "Approve Transaction", confirmationCallback);}

void ui_approval_celo_activate_flow(void) {
    fill_activate();
    nbgl_useCaseReview(TYPE_TRANSACTION, &tagValueList, &ICON_APP_CELO, "Review transaction", NULL, "Approve Transaction", confirmationCallback);
}

void ui_approval_celo_vote_revoke_flow(void) {
    fill_vote_revoke();
    nbgl_useCaseReview(TYPE_TRANSACTION, &tagValueList, &ICON_APP_CELO, "Review transaction", NULL, "Approve Transaction", confirmationCallback);
}

#endif // HAVE_NBGL
