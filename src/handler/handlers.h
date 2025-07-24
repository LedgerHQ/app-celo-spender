#pragma once
#include "parser.h"

// Handler function declarations
int handler_get_public_key(const command_t *cmd);
int handler_sign(const command_t *cmd);
int handler_get_app_configuration(const command_t *cmd);
int handler_sign_personal_message(const command_t *cmd);
int handler_provide_erc20_token_information(const command_t *cmd);
int handler_get_app_type(const command_t *cmd);

#ifndef HAVE_WALLET_ID_SDK
int handler_get_wallet_id(const command_t *cmd);
#endif

uint16_t handleSignEIP712Message_v0(uint8_t p1, const uint8_t *dataBuffer, uint8_t dataLength);