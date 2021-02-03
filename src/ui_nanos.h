#pragma once

#include "ux.h"
#include "ui_common.h"
#include "os_io_seproxyhal.h"

// display stepped screens
extern unsigned int ux_step;
extern unsigned int ux_step_count;

extern const ux_menu_entry_t menu_main[];

// ui_data_selector
extern const bagl_element_t ui_data_selector_nanos[7];
unsigned int ui_data_selector_prepro(const bagl_element_t *element);
unsigned int ui_data_selector_nanos_button(unsigned int button_mask, unsigned int button_mask_counter);

// ui_data_parameter
extern const bagl_element_t ui_data_parameter_nanos[7];
unsigned int ui_data_parameter_prepro(const bagl_element_t *element);
unsigned int ui_data_parameter_nanos_button(unsigned int button_mask, unsigned int button_mask_counter);

extern const bagl_element_t ui_approval_signMessage_nanos[7];
unsigned int ui_approval_signMessage_prepro(const bagl_element_t *element);
unsigned int ui_approval_signMessage_nanos_button(unsigned int button_mask, unsigned int button_mask_counter);

extern const bagl_element_t ui_approval_nanos[19];
unsigned int ui_approval_prepro(const bagl_element_t* element);
unsigned int ui_approval_nanos_button(unsigned int button_mask, unsigned int button_mask_counter);

extern const bagl_element_t ui_address_nanos[7];
unsigned int ui_address_prepro(const bagl_element_t* element);
unsigned int ui_address_nanos_button(unsigned int button_mask, unsigned int button_mask_counter);