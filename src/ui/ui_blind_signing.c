#include "ui_common.h"
#include "icons.h"

#ifdef SCREEN_SIZE_WALLET
static void ui_error_blind_signing_choice(bool confirm __attribute__((unused))) {
    // if (confirm) {
    //     ui_settings();
    // } else {
    ui_idle();
    // }
}
#endif

void ui_error_blind_signing(void) {
#ifdef SCREEN_SIZE_WALLET
    nbgl_useCaseChoice(&ICON_APP_WARNING,
                       "This transaction cannot be clear-signed",
                       "Enable blind signing in the settings to sign this transaction.",
                       "Reject transaction",
                       "Reject transaction",
                       ui_error_blind_signing_choice);
#else
    nbgl_useCaseAction(&C_Alert_circle_14px,
                       "Blind signing must\nbe enabled in\nsettings",
                       NULL,
                       ui_idle);
#endif
}
