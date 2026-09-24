#include "swap_validation.h"

#include <string.h>

#include "strcase.h"
#include "uint256.h"

// Exchange's coin configuration carries a bare ticker, while the app renders
// native amounts with a leading space in front of the coin name.
static const char* ticker_name(const char* ticker) {
    while (*ticker == ' ') {
        ticker++;
    }
    return ticker;
}

// Swap mode signs without any review, so the method has to be one Exchange can
// have approved: a native transfer with no calldata, or a plain token transfer.
// Anything else carries bytes nobody saw. The staking selectors are the reason
// this is a check and not an assertion: finalizeParsing() fills destination and
// value from their calldata, so a vote() whose group and amount happened to
// match the approved ones would otherwise satisfy every comparison below.
static bool method_is_approvable(const swap_tx_t* tx) {
    switch (tx->provisionType) {
        case PROVISION_NONE:
            return !tx->dataPresent;
        case PROVISION_TOKEN:
            // A comment is signed but never displayed, and in swap mode there
            // is no screen it could have appeared on.
            return memcmp(tx->selector, TOKEN_TRANSFER_ID, SELECTOR_LEN) == 0;
        case PROVISION_LOCK:
        case PROVISION_VOTE:
        case PROVISION_ACTIVATE:
        case PROVISION_REVOKE:
        case PROVISION_UNLOCK:
        case PROVISION_WITHDRAW:
        case PROVISION_RELOCK:
        case PROVISION_CREATE_ACCOUNT:
            return false;
    }
    return false;
}

bool swap_tx_matches_approval(const swap_tx_t* tx,
                              const swap_approved_t* approved) {
    if (!method_is_approvable(tx)) {
        return false;
    }

    // The amount below means nothing without the asset: the same number of base
    // units of any provisioned token, or of native CELO, would satisfy it.
    if (_strcasecmp(ticker_name(tx->ticker), ticker_name(approved->ticker)) !=
        0) {
        return false;
    }
    if (tx->decimals != approved->decimals) {
        return false;
    }

    // The approved fee is denominated in the native coin, so a fee paid in a
    // token cannot be compared against it at all.
    if (tx->feeCurrencyLength != 0) {
        return false;
    }

    // Contract creation has no recipient to compare against.
    if (tx->destinationLength != MAX_ADDRESS) {
        return false;
    }
    if (_strcasecmp(tx->destination, approved->recipient) != 0) {
        return false;
    }

    if (tx->value->length != approved->amount_length) {
        return false;
    }
    if (memcmp(tx->value->value, approved->amount, approved->amount_length) !=
        0) {
        return false;
    }

    uint256_t gasPrice, startGas, fees, approvedFees;
    convertUint256BE(tx->gasprice->value, tx->gasprice->length, &gasPrice);
    convertUint256BE(tx->startgas->value, tx->startgas->length, &startGas);
    mul256(&gasPrice, &startGas, &fees);
    convertUint256BE(approved->fee, approved->fee_length, &approvedFees);
    if (gt256(&fees, &approvedFees)) {
        return false;
    }

    return true;
}
