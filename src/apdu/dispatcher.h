#pragma once

#include "parser.h"

/**
 * Dispatch structured APDU command to handler
 *
 * @param[in] cmd Structured command to dispatch
 *
 * @return 0 on success, negative value on error
 */
int apdu_dispatcher(const command_t *cmd);