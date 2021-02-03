#include "bolos_target.h"

#if defined(TARGET_NANOS) && !defined(HAVE_UX_FLOW)

#include "os.h"

#include "ui_nanos.h"
#include "globals.h"

ux_state_t ux;

// display stepped screens
unsigned int ux_step;
unsigned int ux_step_count;

const ux_menu_entry_t menu_settings[];
const ux_menu_entry_t menu_settings_data[];
const ux_menu_entry_t menu_settings_details[];

void ui_idle(void) {
  UX_MENU_DISPLAY(0, menu_main, NULL);
}

#ifdef HAVE_U2F

// change the setting
void menu_settings_data_change(unsigned int enabled) {
  dataAllowed = enabled;
  nvm_write(&N_storage.dataAllowed, (void*)&dataAllowed, sizeof(uint8_t));
  // go back to the menu entry
  UX_MENU_DISPLAY(0, menu_settings, NULL);
}

void menu_settings_details_change(unsigned int enabled) {
  contractDetails = enabled;
  nvm_write(&N_storage.contractDetails, (void*)&contractDetails, sizeof(uint8_t));
  // go back to the menu entry
  UX_MENU_DISPLAY(0, menu_settings, NULL);
}

// show the currently activated entry
void menu_settings_data_init(unsigned int ignored) {
  UNUSED(ignored);
  UX_MENU_DISPLAY(N_storage.dataAllowed?1:0, menu_settings_data, NULL);
}

void menu_settings_details_init(unsigned int ignored) {
  UNUSED(ignored);
  UX_MENU_DISPLAY(N_storage.contractDetails?1:0, menu_settings_details, NULL);
}

const ux_menu_entry_t menu_settings_data[] = {
  {NULL, menu_settings_data_change, 0, NULL, "No", NULL, 0, 0},
  {NULL, menu_settings_data_change, 1, NULL, "Yes", NULL, 0, 0},
  UX_MENU_END
};

const ux_menu_entry_t menu_settings_details[] = {
  {NULL, menu_settings_details_change, 0, NULL, "No", NULL, 0, 0},
  {NULL, menu_settings_details_change, 1, NULL, "Yes", NULL, 0, 0},
  UX_MENU_END
};

const ux_menu_entry_t menu_settings[] = {
  {NULL, menu_settings_data_init, 0, NULL, "Contract data", NULL, 0, 0},
  {NULL, menu_settings_details_init, 0, NULL, "Display data", NULL, 0, 0},
  {menu_main, NULL, 1, &C_icon_back, "Back", NULL, 61, 40},
  UX_MENU_END
};
#endif // HAVE_U2F

const ux_menu_entry_t menu_about[] = {
  {NULL, NULL, 0, NULL, "Version", APPVERSION , 0, 0},
  {menu_main, NULL, 2, &C_icon_back, "Back", NULL, 61, 40},
  UX_MENU_END
};

const ux_menu_entry_t menu_main[] = {
  {NULL, NULL, 0, NULL, "Use wallet to", "view accounts", 0, 0},
  {menu_settings, NULL, 0, NULL, "Settings", NULL, 0, 0},
  {menu_about, NULL, 0, NULL, "About", NULL, 0, 0},
  {NULL, os_sched_exit, 0, &C_icon_dashboard, "Quit app", NULL, 50, 29},
  UX_MENU_END
};

const bagl_element_t ui_address_nanos[] = {
  // type                               userid    x    y   w    h  str rad fill      fg        bg      fid iid  txt   touchparams...       ]
  {{BAGL_RECTANGLE                      , 0x00,   0,   0, 128,  32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0}, NULL, 0, 0, 0, NULL, NULL, NULL},

  {{BAGL_ICON                           , 0x00,   3,  12,   7,   7, 0, 0, 0        , 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CROSS  }, NULL, 0, 0, 0, NULL, NULL, NULL },
  {{BAGL_ICON                           , 0x00, 117,  13,   8,   6, 0, 0, 0        , 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CHECK  }, NULL, 0, 0, 0, NULL, NULL, NULL },

  //{{BAGL_ICON                           , 0x01,  31,   9,  14,  14, 0, 0, 0        , 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_EYE_BADGE  }, NULL, 0, 0, 0, NULL, NULL, NULL },
  {{BAGL_LABELINE                       , 0x01,   0,  12, 128,  12, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 0  }, "Confirm", 0, 0, 0, NULL, NULL, NULL },
  {{BAGL_LABELINE                       , 0x01,   0,  26, 128,  12, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 0  }, "address", 0, 0, 0, NULL, NULL, NULL },

  {{BAGL_LABELINE                       , 0x02,   0,  12, 128,  12, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px|BAGL_FONT_ALIGNMENT_CENTER, 0  }, "Address", 0, 0, 0, NULL, NULL, NULL },
  {{BAGL_LABELINE                       , 0x02,  23,  26,  82,  12, 0x80|10, 0, 0  , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 26  }, (char*)strings.common.fullAddress, 0, 0, 0, NULL, NULL, NULL },
};

unsigned int ui_address_prepro(const bagl_element_t* element) {
    if (element->component.userid > 0) {
        unsigned int display = (ux_step == element->component.userid-1);
        if(display) {
          switch(element->component.userid) {
          case 1:
            UX_CALLBACK_SET_INTERVAL(2000);
            break;
          case 2:
            UX_CALLBACK_SET_INTERVAL(MAX(3000, 1000+bagl_label_roundtrip_duration_ms(element, 7)));
            break;
          }
        }
        return display;
    }
    return 1;
}

const bagl_element_t ui_approval_nanos[] = {
  // type                               userid    x    y   w    h  str rad fill      fg        bg      fid iid  txt   touchparams...       ]
  {{BAGL_RECTANGLE                      , 0x00,   0,   0, 128,  32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0}, NULL, 0, 0, 0, NULL, NULL, NULL},

  {{BAGL_ICON                           , 0x00,   3,  12,   7,   7, 0, 0, 0        , 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CROSS  }, NULL, 0, 0, 0, NULL, NULL, NULL },
  {{BAGL_ICON                           , 0x00, 117,  13,   8,   6, 0, 0, 0        , 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CHECK  }, NULL, 0, 0, 0, NULL, NULL, NULL },

  //{{BAGL_ICON                           , 0x01,  21,   9,  14,  14, 0, 0, 0        , 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_TRANSACTION_BADGE  }, NULL, 0, 0, 0, NULL, NULL, NULL },
  {{BAGL_LABELINE                       , 0x01,   0,  12, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 0  }, "Confirm", 0, 0, 0, NULL, NULL, NULL },
  {{BAGL_LABELINE                       , 0x01,   0,  26, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 0  }, "transaction", 0, 0, 0, NULL, NULL, NULL },

  {{BAGL_LABELINE, 0x02, 0, 12, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0}, "WARNING", 0, 0, 0, NULL, NULL, NULL},
  {{BAGL_LABELINE, 0x02, 23, 26, 82, 12, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0}, "Data present", 0, 0, 0, NULL, NULL, NULL},

  {{BAGL_LABELINE                       , 0x03,   0,  12, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px|BAGL_FONT_ALIGNMENT_CENTER, 0  }, "Amount", 0, 0, 0, NULL, NULL, NULL },
  {{BAGL_LABELINE                       , 0x03,  23,  26,  82,  12, 0x80|10, 0, 0  , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 26  }, (char*)strings.common.fullAmount, 0, 0, 0, NULL, NULL, NULL },

  {{BAGL_LABELINE                       , 0x04,   0,  12, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px|BAGL_FONT_ALIGNMENT_CENTER, 0  }, "Address", 0, 0, 0, NULL, NULL, NULL },
  {{BAGL_LABELINE                       , 0x04,  23,  26,  82,  12, 0x80|10, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 50   }, (char*)strings.common.fullAddress, 0, 0, 0, NULL, NULL, NULL },

  {{BAGL_LABELINE                       , 0x05,   0,  12, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px|BAGL_FONT_ALIGNMENT_CENTER, 0  }, "Maximum fees", 0, 0, 0, NULL, NULL, NULL },
  {{BAGL_LABELINE                       , 0x05,  23,  26,  82,  12, 0x80|10, 0, 0  , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 26  }, (char*)strings.common.maxFee, 0, 0, 0, NULL, NULL, NULL },

  {{BAGL_LABELINE, 0x06, 0, 12, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0}, "No Gateway Fee", 0, 0, 0, NULL, NULL, NULL},
  {{BAGL_LABELINE, 0x06, 23, 26, 82, 12, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0}, "", 0, 0, 0, NULL, NULL, NULL},

  {{BAGL_LABELINE                       , 0x07,   0,  12, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px|BAGL_FONT_ALIGNMENT_CENTER, 0  }, "Gateway Fee", 0, 0, 0, NULL, NULL, NULL },
  {{BAGL_LABELINE                       , 0x07,  23,  26,  82,  12, 0x80|10, 0, 0  , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 26  }, (char*)strings.common.gatewayFee, 0, 0, 0, NULL, NULL, NULL },

  {{BAGL_LABELINE                       , 0x08,   0,  12, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px|BAGL_FONT_ALIGNMENT_CENTER, 0  }, "Gateway Addr", 0, 0, 0, NULL, NULL, NULL },
  {{BAGL_LABELINE                       , 0x08,  23,  26,  82,  12, 0x80|10, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 50   }, (char*)strings.common.fullGatewayAddress, 0, 0, 0, NULL, NULL, NULL },
};

unsigned int ui_approval_prepro(const bagl_element_t* element) {
    unsigned int display = 1;
    if (element->component.userid > 0) {
        display = (ux_step == element->component.userid-1);
        if(display) {
          switch(element->component.userid) {
          case 1:
            UX_CALLBACK_SET_INTERVAL(2000);
            break;
          case 2:
            if (dataPresent && !N_storage.contractDetails) {
              UX_CALLBACK_SET_INTERVAL(3000);
            }
            else {
              display = 0;
              ux_step++; // display the next step
            }
            break;
          case 3:
            UX_CALLBACK_SET_INTERVAL(MAX(3000, 1000+bagl_label_roundtrip_duration_ms(element, 7)));
            break;
          case 4:
            UX_CALLBACK_SET_INTERVAL(MAX(3000, 1000+bagl_label_roundtrip_duration_ms(element, 7)));
            break;
          case 5:
            UX_CALLBACK_SET_INTERVAL(MAX(3000, 1000+bagl_label_roundtrip_duration_ms(element, 7)));
            break;
          case 6:
            if (tmpContent.txContent.gatewayDestinationLength == 0) {
              UX_CALLBACK_SET_INTERVAL(3000);
            }
            else {
              display = 0;
              ux_step++; // display the next step
            }
          case 7:
            UX_CALLBACK_SET_INTERVAL(MAX(3000, 1000+bagl_label_roundtrip_duration_ms(element, 7)));
            break;
          case 8:
            UX_CALLBACK_SET_INTERVAL(MAX(3000, 1000+bagl_label_roundtrip_duration_ms(element, 7)));
            break;
          }
        }
    }
    return display;
}

unsigned int ui_approval_nanos_button(unsigned int button_mask, unsigned int button_mask_counter);

const bagl_element_t ui_approval_signMessage_nanos[] = {
  // type                               userid    x    y   w    h  str rad fill      fg        bg      fid iid  txt   touchparams...       ]
  {{BAGL_RECTANGLE                      , 0x00,   0,   0, 128,  32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0}, NULL, 0, 0, 0, NULL, NULL, NULL},

  {{BAGL_ICON                           , 0x00,   3,  12,   7,   7, 0, 0, 0        , 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CROSS  }, NULL, 0, 0, 0, NULL, NULL, NULL },
  {{BAGL_ICON                           , 0x00, 117,  13,   8,   6, 0, 0, 0        , 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CHECK  }, NULL, 0, 0, 0, NULL, NULL, NULL },

  //{{BAGL_ICON                           , 0x01,  28,   9,  14,  14, 0, 0, 0        , 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_TRANSACTION_BADGE  }, NULL, 0, 0, 0, NULL, NULL, NULL },
  {{BAGL_LABELINE                       , 0x01,   0,  12, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 0  }, "Sign the", 0, 0, 0, NULL, NULL, NULL },
  {{BAGL_LABELINE                       , 0x01,   0,  26, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 0  }, "message", 0, 0, 0, NULL, NULL, NULL },

  {{BAGL_LABELINE                       , 0x02,   0,  12, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px|BAGL_FONT_ALIGNMENT_CENTER, 0  }, "Message hash", 0, 0, 0, NULL, NULL, NULL },
  {{BAGL_LABELINE                       , 0x02,   0,  26, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 0  }, strings.common.fullAddress, 0, 0, 0, NULL, NULL, NULL },

};

unsigned int
ui_approval_signMessage_nanos_button(unsigned int button_mask, unsigned int button_mask_counter);

unsigned int ui_approval_signMessage_prepro(const bagl_element_t *element) {
    if (element->component.userid > 0) {
        switch (element->component.userid) {
        case 1:
            UX_CALLBACK_SET_INTERVAL(2000);
            break;
        case 2:
            UX_CALLBACK_SET_INTERVAL(3000);
            break;
        }
        return (ux_step == element->component.userid - 1);
    }
    return 1;
}

const bagl_element_t ui_data_selector_nanos[] = {
    // type                               userid    x    y   w    h  str rad
    // fill      fg        bg      fid iid  txt   touchparams...       ]
    {{BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0}, NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_ICON, 0x00, 3, 12, 7, 7, 0, 0, 0, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CROSS}, NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_ICON, 0x00, 117, 13, 8, 6, 0, 0, 0, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CHECK}, NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x01, 0, 12, 128, 12, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0}, "Confirm", 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x01, 0, 26, 128, 12, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0}, "selector", 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_LABELINE, 0x02, 0, 12, 128, 12, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0}, "Selector", 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x02, 23, 26, 82, 12, 0x80 | 10, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 26}, (char *)strings.tmp.tmp, 0, 0, 0, NULL, NULL, NULL},
};

unsigned int ui_data_selector_prepro(const bagl_element_t *element) {
    if (element->component.userid > 0) {
        unsigned int display = (ux_step == element->component.userid - 1);
        if (display) {
            switch (element->component.userid) {
                case 1:
                    UX_CALLBACK_SET_INTERVAL(2000);
                    break;
                case 2:
                    UX_CALLBACK_SET_INTERVAL(MAX(
                        3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
                    break;
            }
        }
        return display;
    }
    return 1;
}

unsigned int ui_data_selector_nanos_button(unsigned int button_mask,
                                     unsigned int button_mask_counter);

const bagl_element_t ui_data_parameter_nanos[] = {
    // type                               userid    x    y   w    h  str rad
    // fill      fg        bg      fid iid  txt   touchparams...       ]
    {{BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0}, NULL, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_ICON, 0x00, 3, 12, 7, 7, 0, 0, 0, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CROSS}, NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_ICON, 0x00, 117, 13, 8, 6, 0, 0, 0, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CHECK}, NULL, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_LABELINE, 0x01, 0, 12, 128, 12, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0}, "Confirm", 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x01, 0, 26, 128, 12, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0}, "parameter", 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_LABELINE, 0x02, 0, 12, 128, 12, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0}, (char*)strings.tmp.tmp2, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x02, 23, 26, 82, 12, 0x80 | 10, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 26}, (char *)strings.tmp.tmp, 0, 0, 0, NULL, NULL, NULL},
};

unsigned int ui_data_parameter_prepro(const bagl_element_t *element) {
    if (element->component.userid > 0) {
        unsigned int display = (ux_step == element->component.userid - 1);
        if (display) {
            switch (element->component.userid) {
            case 1:
                UX_CALLBACK_SET_INTERVAL(2000);
                break;
            case 2:
                UX_CALLBACK_SET_INTERVAL(MAX(
                    3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
                break;
            }
        }
        return display;
    }
    return 1;
}

unsigned int ui_data_parameter_nanos_button(unsigned int button_mask,
                                     unsigned int button_mask_counter);

unsigned int ui_address_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
    UNUSED(button_mask_counter);
    switch(button_mask) {
        case BUTTON_EVT_RELEASED|BUTTON_LEFT: // CANCEL
			      io_seproxyhal_touch_address_cancel(NULL);
            break;

        case BUTTON_EVT_RELEASED|BUTTON_RIGHT: { // OK
			      io_seproxyhal_touch_address_ok(NULL);
			      break;
        }
    }
    return 0;
}

unsigned int ui_approval_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
    UNUSED(button_mask_counter);
    switch(button_mask) {
        case BUTTON_EVT_RELEASED|BUTTON_LEFT:
            io_seproxyhal_touch_tx_cancel(NULL);
            break;

        case BUTTON_EVT_RELEASED|BUTTON_RIGHT: {
			      io_seproxyhal_touch_tx_ok(NULL);
            break;
        }
    }
    return 0;
}


unsigned int ui_approval_signMessage_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
    UNUSED(button_mask_counter);
    switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        io_seproxyhal_touch_signMessage_cancel(NULL);
        break;

    case BUTTON_EVT_RELEASED | BUTTON_RIGHT: {
        io_seproxyhal_touch_signMessage_ok(NULL);
        break;
    }
    }
    return 0;
}

unsigned int ui_data_selector_nanos_button(unsigned int button_mask,
                                     unsigned int button_mask_counter) {
   UNUSED(button_mask_counter);
   switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        io_seproxyhal_touch_data_cancel(NULL);
        break;

    case BUTTON_EVT_RELEASED | BUTTON_RIGHT: {
        io_seproxyhal_touch_data_ok(NULL);
        break;
    }
    }
    return 0;
}

unsigned int ui_data_parameter_nanos_button(unsigned int button_mask,
                                     unsigned int button_mask_counter) {
   UNUSED(button_mask_counter);
   switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        io_seproxyhal_touch_data_cancel(NULL);
        break;

    case BUTTON_EVT_RELEASED | BUTTON_RIGHT: {
        io_seproxyhal_touch_data_ok(NULL);
        break;
    }
    }
    return 0;
}

#endif // #if defined(TARGET_NANOS) && !defined(HAVE_UX_FLOW)
