#pragma once

#include "bolos_target.h"

#include "ux.h"

#ifdef HAVE_NBGL
#include "nbgl_use_case.h"

extern nbgl_contentTagValue_t tagValuePair[5];
extern nbgl_contentTagValueList_t tagValueList;
extern nbgl_contentInfoLongPress_t infoLongPress;
#endif  // HAVE_NBGL

void ui_idle(void);
void ui_display_address(void);
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

// EIP-712
void ui_712_start(void);
void ui_712_start_unfiltered(void);
void ui_712_switch_to_message(void);
void ui_712_switch_to_sign(void);
void ui_sign_712_v0(void);
void ui_error_blind_signing(void);

unsigned int io_seproxyhal_touch_data_ok(void);
unsigned int io_seproxyhal_touch_data_cancel(void);

unsigned int io_seproxyhal_touch_address_ok(void);
unsigned int io_seproxyhal_touch_address_cancel(void);

unsigned int io_seproxyhal_touch_tx_ok(void);
unsigned int io_seproxyhal_touch_tx_cancel(void);

unsigned int io_seproxyhal_touch_signMessage_ok(void);

/**
 * @brief Sends a status response via the SEPROXYHAL interface.
 *
 * This function sends an APDU response with a status word and optional data.
 * It can optionally reset the application context and return to the idle UI state.
 *
 * @param[in] sw The status word to send (2 bytes)
 * @param[in] tx The current transaction offset in the APDU buffer
 * @param[in] reset Whether to reset the application context before sending
 * @param[in] idle Whether to return to the idle UI state after sending
 * @return The error code from the io_exchange operation
 *
 * @note This function modifies the global APDU buffer (G_io_apdu_buffer)
 * @note The status word is encoded in big-endian format using U2BE_ENCODE
 * @note If reset is true, the application context will be cleared via reset_app_context()
 * @note If idle is true, the UI will return to idle state via ui_idle()
 */
uint16_t io_seproxyhal_send_status(uint16_t sw, uint32_t tx, bool reset, bool idle);