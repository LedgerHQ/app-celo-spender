#ifdef HAVE_BAGL
#include "bolos_target.h"

#include "ui_flow.h"
#include "globals.h"

void ui_idle(void) {
    // reserve a display stack slot if none yet
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, ux_idle_flow, NULL);
}

void display_settings(void);
void switch_settings_contract_data(void);
void switch_settings_display_data(void);

//////////////////////////////////////////////////////////////////////
UX_STEP_NOCB(
    ux_idle_flow_1_step,
    nn, //pnn,
    {
      //"", //&C_icon_dashboard,
      "Application",
      "is ready",
    });
UX_STEP_NOCB(
    ux_idle_flow_2_step,
    bn,
    {
      "Version",
      APPVERSION,
    });
UX_STEP_CB(
    ux_idle_flow_3_step,
    pb,
    display_settings(),
    {
      &C_icon_eye,
      "Settings",
    });
UX_STEP_CB(
    ux_idle_flow_4_step,
    pb,
    os_sched_exit(-1),
    {
      &C_icon_dashboard_x,
      "Quit",
    });

UX_FLOW(ux_idle_flow,
  &ux_idle_flow_1_step,
  &ux_idle_flow_2_step,
  &ux_idle_flow_3_step,
  &ux_idle_flow_4_step,
  FLOW_LOOP
);

#define SETTINGS_TEXT_SIZE 14

static char g_SettingsText[SETTINGS_TEXT_SIZE];

#if defined(TARGET_NANOS)

UX_STEP_CB_INIT(
    ux_settings_flow_1_step,
    bnnn_paging,
    {
      const char *text = N_storage.dataAllowed ? "Allowed" : "NOT Allowed";
      strlcpy(g_SettingsText, text, SETTINGS_TEXT_SIZE);
    },
    switch_settings_contract_data(),
    {
      .title = "Contract data",
      .text = g_SettingsText,
    });

UX_STEP_CB_INIT(
    ux_settings_flow_2_step,
    bnnn_paging,
    {
      const char *text = N_storage.contractDetails ? "Displayed" : "NOT Displayed";
      strlcpy(g_SettingsText, text, SETTINGS_TEXT_SIZE);
    },
    switch_settings_display_data(),
    {
      .title = "Debug data",
      .text = g_SettingsText,
    });

#else

UX_STEP_CB_INIT(
    ux_settings_flow_1_step,
    bnnn,
    {
      const char *text = N_storage.dataAllowed ? "Allowed" : "NOT Allowed";
      strlcpy(g_SettingsText, text, SETTINGS_TEXT_SIZE);
    },
    switch_settings_contract_data(),
    {
      "Contract data",
      "Allow contract data",
      "in transactions",
      g_SettingsText
    });

UX_STEP_CB_INIT(
    ux_settings_flow_2_step,
    bnnn,
    {
      const char *text = N_storage.contractDetails ? "Displayed" : "NOT Displayed";
      strlcpy(g_SettingsText, text, SETTINGS_TEXT_SIZE);
    },
    switch_settings_display_data(),
    {
      "Debug data",
      "Display contract data",
      "details",
      g_SettingsText
    });

#endif

UX_STEP_CB(
    ux_settings_flow_3_step,
    pb,
    ui_idle(),
    {
      &C_icon_back_x,
      "Back",
    });

UX_FLOW(ux_settings_flow,
  &ux_settings_flow_1_step,
  &ux_settings_flow_2_step,
  &ux_settings_flow_3_step
);

void display_settings() {
  ux_flow_init(0, ux_settings_flow, NULL);
}

void switch_settings_contract_data() {
  uint8_t value = (N_storage.dataAllowed ? 0 : 1);
  nvm_write(&N_storage.dataAllowed, (void*)&value, sizeof(uint8_t));
  display_settings();
}

void switch_settings_display_data() {
  uint8_t value = (N_storage.contractDetails ? 0 : 1);
  nvm_write(&N_storage.contractDetails, (void*)&value, sizeof(uint8_t));
  display_settings();
}

//////////////////////////////////////////////////////////////////////
UX_STEP_NOCB(
    ux_display_public_flow_1_step,
    pnn,
    {
      &C_icon_eye,
      "Verify",
      "address",
    });
UX_STEP_NOCB(
    ux_display_public_flow_2_step,
    bnnn_paging,
    {
      .title = "Address",
      .text = strings.common.fullAddress,
    });
UX_STEP_CB(
    ux_display_public_flow_3_step,
    pb,
    io_seproxyhal_touch_address_ok(),
    {
      &C_icon_validate_14,
      "Approve",
    });
UX_STEP_CB(
    ux_display_public_flow_4_step,
    pb,
    io_seproxyhal_touch_address_cancel(),
    {
      &C_icon_crossmark,
      "Reject",
    });

UX_FLOW(ux_display_public_flow,
  &ux_display_public_flow_1_step,
  &ux_display_public_flow_2_step,
  &ux_display_public_flow_3_step,
  &ux_display_public_flow_4_step
);

//////////////////////////////////////////////////////////////////////
UX_STEP_NOCB(
    ux_confirm_selector_flow_1_step,
    pnn,
    {
      &C_icon_eye,
      "Verify",
      "selector",
    });

UX_STEP_NOCB(
    ux_confirm_selector_flow_2_step,
    bn,
    {
      "Selector",
      strings.tmp.tmp
    });
UX_STEP_CB(
    ux_confirm_selector_flow_3_step,
    pb,
    io_seproxyhal_touch_data_ok(),
    {
      &C_icon_validate_14,
      "Approve",
    });
UX_STEP_CB(
    ux_confirm_selector_flow_4_step,
    pb,
    io_seproxyhal_touch_data_cancel(),
    {
      &C_icon_crossmark,
      "Reject",
    });

UX_FLOW(ux_confirm_selector_flow,
  &ux_confirm_selector_flow_1_step,
  &ux_confirm_selector_flow_2_step,
  &ux_confirm_selector_flow_3_step,
  &ux_confirm_selector_flow_4_step
);

//////////////////////////////////////////////////////////////////////
UX_STEP_NOCB(
    ux_confirm_parameter_flow_1_step,
    pnn,
    {
      &C_icon_eye,
      "Verify",
      strings.tmp.tmp2
    });

UX_STEP_NOCB(
    ux_confirm_parameter_flow_2_step,
    bnnn_paging,
    {
      .title = "Parameter",
      .text = strings.tmp.tmp,
    });

UX_STEP_CB(
    ux_confirm_parameter_flow_3_step,
    pb,
    io_seproxyhal_touch_data_ok(),
    {
      &C_icon_validate_14,
      "Approve",
    });

UX_STEP_CB(
    ux_confirm_parameter_flow_4_step,
    pb,
    io_seproxyhal_touch_data_cancel(),
    {
      &C_icon_crossmark,
      "Reject",
    });

UX_FLOW(ux_confirm_parameter_flow,
  &ux_confirm_parameter_flow_1_step,
  &ux_confirm_parameter_flow_2_step,
  &ux_confirm_parameter_flow_3_step,
  &ux_confirm_parameter_flow_4_step
);

//////////////////////////////////////////////////////////////////////
UX_STEP_NOCB(ux_approval_tx_1_step,
    pnn,
    {
      &C_icon_eye,
      "Review",
      "transaction",
    });

UX_STEP_NOCB(
    ux_approval_tx_2_step,
    bnnn_paging,
    {
      .title = "Amount",
      .text = strings.common.fullAmount
    });

UX_STEP_NOCB(
    ux_approval_tx_3_step,
    bnnn_paging,
    {
      .title = "To",
      .text = strings.common.fullAddress,
    });

UX_STEP_NOCB(
    ux_approval_tx_4_step,
    bnnn_paging,
    {
      .title = "Max Fees",
      .text = strings.common.maxFee,
    });

UX_STEP_NOCB(
    ux_celo_approval_tx_gateway_fee_step,
    bnnn_paging,
    {
      .title = "Gateway Fee",
      .text = strings.common.gatewayFee,
    });

UX_STEP_NOCB(
    ux_celo_approval_staking_type_step,
    bnnn_paging,
    {
      .title = "Type",
      .text = strings.common.stakingType,
    });

UX_STEP_NOCB(
    ux_celo_approval_staking_validator_step,
    bnnn_paging,
    {
      .title = "Validator",
      .text = strings.common.fullAddress,
    });

UX_STEP_NOCB(
    ux_celo_approval_tx_gateway_address_step,
    bnnn_paging,
    {
      .title = "Gateway Addr",
      .text = strings.common.fullGatewayAddress,
    });

UX_STEP_CB(
    ux_approval_tx_5_step,
    pbb,
    io_seproxyhal_touch_tx_ok(),
    {
      &C_icon_validate_14,
      "Accept",
      "and send",
    });
UX_STEP_CB(
    ux_approval_tx_6_step,
    pb,
    io_seproxyhal_touch_tx_cancel(),
    {
      &C_icon_crossmark,
      "Reject",
    });

UX_STEP_NOCB(ux_approval_tx_data_warning_step,
    pbb,
    {
      &C_icon_warning,
      "Data",
      "Present",
    });

UX_FLOW(ux_approval_celo_tx_flow,
  &ux_approval_tx_1_step,
  &ux_approval_tx_2_step,
  &ux_approval_tx_3_step,
  &ux_approval_tx_4_step,
  &ux_approval_tx_5_step,
  &ux_approval_tx_6_step
);

UX_FLOW(ux_approval_celo_gateway_tx_flow,
  &ux_approval_tx_1_step,
  &ux_approval_tx_2_step,
  &ux_approval_tx_3_step,
  &ux_approval_tx_4_step,
  &ux_celo_approval_tx_gateway_fee_step,
  &ux_celo_approval_tx_gateway_address_step,
  &ux_approval_tx_5_step,
  &ux_approval_tx_6_step
);

UX_FLOW(ux_approval_celo_data_warning_tx_flow,
  &ux_approval_tx_1_step,
  &ux_approval_tx_data_warning_step,
  &ux_approval_tx_2_step,
  &ux_approval_tx_3_step,
  &ux_approval_tx_4_step,
  &ux_approval_tx_5_step,
  &ux_approval_tx_6_step
);

UX_FLOW(ux_approval_celo_data_warning_gateway_tx_flow,
  &ux_approval_tx_1_step,
  &ux_approval_tx_data_warning_step,
  &ux_approval_tx_2_step,
  &ux_approval_tx_3_step,
  &ux_approval_tx_4_step,
  &ux_celo_approval_tx_gateway_fee_step,
  &ux_celo_approval_tx_gateway_address_step,
  &ux_approval_tx_5_step,
  &ux_approval_tx_6_step
);

UX_FLOW(ux_approval_celo_lock_unlock_flow,
  &ux_approval_tx_1_step,
  &ux_celo_approval_staking_type_step,
  &ux_approval_tx_2_step,
  &ux_approval_tx_4_step,
  &ux_approval_tx_5_step,
  &ux_approval_tx_6_step
);

UX_FLOW(ux_approval_celo_withdraw_flow,
  &ux_approval_tx_1_step,
  &ux_celo_approval_staking_type_step,
  &ux_approval_tx_4_step,
  &ux_approval_tx_5_step,
  &ux_approval_tx_6_step
);

UX_FLOW(ux_approval_celo_vote_revoke_flow,
  &ux_approval_tx_1_step,
  &ux_celo_approval_staking_type_step,
  &ux_approval_tx_2_step,
  &ux_celo_approval_staking_validator_step,
  &ux_approval_tx_4_step,
  &ux_approval_tx_5_step,
  &ux_approval_tx_6_step
);

UX_FLOW(ux_approval_celo_activate_flow,
  &ux_approval_tx_1_step,
  &ux_celo_approval_staking_type_step,
  &ux_celo_approval_staking_validator_step,
  &ux_approval_tx_4_step,
  &ux_approval_tx_5_step,
  &ux_approval_tx_6_step
);

UX_FLOW(ux_approval_celo_relock_flow,
  &ux_approval_tx_1_step,
  &ux_celo_approval_staking_type_step,
  &ux_approval_tx_2_step,
  &ux_approval_tx_4_step,
  &ux_approval_tx_5_step,
  &ux_approval_tx_6_step
);

UX_FLOW(ux_approval_celo_create_account_flow,
  &ux_approval_tx_1_step,
  &ux_celo_approval_staking_type_step,
  &ux_approval_tx_4_step,
  &ux_approval_tx_5_step,
  &ux_approval_tx_6_step
);

//////////////////////////////////////////////////////////////////////
UX_STEP_NOCB(
    ux_sign_flow_1_step,
    pnn,
    {
      &C_icon_certificate,
      "Sign",
      "message",
    });

UX_STEP_NOCB(
    ux_sign_flow_2_step,
    bnnn_paging,
    {
      .title = "Message hash",
      .text = strings.common.fullAddress,
    });

UX_STEP_CB(
    ux_sign_flow_3_step,
    pbb,
    io_seproxyhal_touch_signMessage_ok(),
    {
      &C_icon_validate_14,
      "Sign",
      "message",
    });

UX_STEP_CB(
    ux_sign_flow_4_step,
    pbb,
    io_seproxyhal_touch_signMessage_cancel(),
    {
      &C_icon_crossmark,
      "Cancel",
      "signature",
    });

UX_FLOW(ux_sign_flow,
  &ux_sign_flow_1_step,
  &ux_sign_flow_2_step,
  &ux_sign_flow_3_step,
  &ux_sign_flow_4_step
);
#endif // HAVE_BAGL
