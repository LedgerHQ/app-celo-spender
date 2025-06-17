#include "get_app_info.h"

#include "io.h"
#include "sw.h"
#include "globals.h"
#include "constants.h"


/**
 * Handles the retrieval of the application configuration.
 *
 * @param p1 The first parameter of the APDU command.
 * @param p2 The second parameter of the APDU command.
 * @param workBuffer The buffer containing the command data.
 * @param dataLength The length of the command data.
 * @param flags The flags variable.
 * @param tx The transaction variable.
 */
void handleGetAppConfiguration(uint8_t p1, uint8_t p2, uint8_t *workBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
  UNUSED(p1);
  UNUSED(p2);
  UNUSED(workBuffer);
  UNUSED(dataLength);
  UNUSED(flags);
  G_io_apdu_buffer[0] = (N_storage.dataAllowed ? APP_FLAG_DATA_ALLOWED : 0x00);
#ifndef HAVE_TOKENS_LIST
  G_io_apdu_buffer[0] |= APP_FLAG_EXTERNAL_TOKEN_NEEDED;
#endif
  G_io_apdu_buffer[1] = MAJOR_VERSION;
  G_io_apdu_buffer[2] = MINOR_VERSION;
  G_io_apdu_buffer[3] = PATCH_VERSION;
  *tx = 4;
  THROW(SW_OK);
}

/**
 * Handles the retrieval of the application type.
 *
 * @param p1 The first parameter of the APDU command.
 * @param p2 The second parameter of the APDU command.
 * @param workBuffer The buffer containing the command data.
 * @param dataLength The length of the command data.
 * @param flags The flags variable.
 * @param tx The transaction variable.
 */
void handleGetAppType(uint8_t p1, uint8_t p2, uint8_t *workBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
  UNUSED(p1);
  UNUSED(p2);
  UNUSED(workBuffer);
  UNUSED(dataLength);
  UNUSED(flags);
  G_io_apdu_buffer[0] = APP_TYPE;
  *tx = 1;
  THROW(SW_OK);
}


