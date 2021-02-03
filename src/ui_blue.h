#pragma once

#include "os_io_seproxyhal.h"
#include "glyphs.h"
#include "ui_common.h"

// display stepped screens
extern unsigned int ux_step;
extern unsigned int ux_step_count;

#define COLOR_BG_1 0xF9F9F9
#define COLOR_APP 0x0ebdcf
#define COLOR_APP_LIGHT 0x87dee6

extern const bagl_element_t ui_data_parameter_blue[11];
unsigned int ui_data_parameter_blue_prepro(const bagl_element_t* element);
unsigned int ui_data_parameter_blue_button(unsigned int button_mask, unsigned int button_mask_counter);

extern const bagl_element_t ui_address_blue[8];
unsigned int ui_address_blue_prepro(const bagl_element_t* element);
unsigned int ui_address_blue_button(unsigned int button_mask, unsigned int button_mask_counter);

extern const bagl_element_t ui_data_selector_blue[7];
unsigned int ui_data_selector_blue_prepro(const bagl_element_t* element);
unsigned int ui_data_selector_blue_button(unsigned int button_mask, unsigned int button_mask_counter);

extern const bagl_element_t ui_idle_blue[9];
const bagl_element_t *ui_idle_blue_prepro(const bagl_element_t *element);
unsigned int ui_idle_blue_button(unsigned int button_mask, unsigned int button_mask_counter);
