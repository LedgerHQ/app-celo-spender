#pragma once

#include <stdint.h>
#include "bolos_target.h"

#include "ux.h"

#ifdef HAVE_NBGL
#include "nbgl_use_case.h"

extern nbgl_layoutTagValue_t tagValuePair[5];
extern nbgl_layoutTagValueList_t tagValueList;
extern nbgl_pageInfoLongPress_t infoLongPress;
#endif // HAVE_NBGL

#if defined(TARGET_STAX) || defined(TARGET_FLEX)
#define ICON_APP_CELO        C_celo_64px
#define ICON_APP_HOME        ICON_APP_CELO
#define ICON_APP_WARNING     LARGE_WARNING_ICON
#elif defined(TARGET_APEX_P)
#define ICON_APP_CELO        C_celo_48px
#define ICON_APP_HOME        ICON_APP_CELO
#define ICON_APP_WARNING     LARGE_WARNING_ICON
#endif

void ui_idle(void);  // must be implemented by each ui handler
void ui_start_page(uint8_t page);
void ui_settings(void);
void ui_display_public_flow(void);
void ui_display_sign_flow(void);
void ui_confirm_selector_flow(void);
void ui_confirm_parameter_flow(void);
void ui_approval_celo_lock_unlock_flow(void);
void ui_approval_celo_withdraw_flow(void);
void ui_approval_celo_vote_revoke_flow(void);
void ui_approval_celo_activate_flow(void);
void ui_approval_celo_relock_flow(void);
void ui_approval_celo_create_account_flow(void);
void ui_approval_celo_data_warning_gateway_tx_flow(void);
void ui_approval_celo_gateway_tx_flow(void);
void ui_approval_celo_data_warning_tx_flow(void);
void ui_approval_celo_tx_flow(void);
void ui_error_blind_signing(void);

unsigned int io_seproxyhal_touch_data_ok(void);
unsigned int io_seproxyhal_touch_data_cancel(void);

unsigned int io_seproxyhal_touch_address_ok(void);
unsigned int io_seproxyhal_touch_address_cancel(void);

unsigned int io_sign_and_send_tx(void);
unsigned int io_seproxyhal_touch_tx_ok(void);
unsigned int io_seproxyhal_touch_tx_cancel(void);

unsigned int io_seproxyhal_touch_signMessage_ok(void);
unsigned int io_seproxyhal_touch_signMessage_cancel(void);
