/*******************************************************************************
*   Ledger Ethereum App
*   (c) 2016-2019 Ledger
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "os.h"
#include "cx.h"
#include "ethUstream.h"
#include "ethUtils.h"
#include "uint256.h"
#include "tokens.h"
#include "chainConfig.h"
#include "celo.h"

#include "os_io_seproxyhal.h"

#include "globals.h"
#include "utils.h"

#include "ui_flow.h"

uint8_t G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

#define APP_FLAG_DATA_ALLOWED 0x01
#define APP_FLAG_EXTERNAL_TOKEN_NEEDED 0x02

#define APP_TYPE 0x02

#define CLA 0xE0
#define INS_GET_PUBLIC_KEY 0x02
#define INS_SIGN 0x04
#define INS_GET_APP_CONFIGURATION 0x06
#define INS_SIGN_PERSONAL_MESSAGE 0x08
#define INS_PROVIDE_ERC20_TOKEN_INFORMATION 0x0A
#define INS_GET_APP_TYPE 0x0C
#define P1_CONFIRM 0x01
#define P1_NON_CONFIRM 0x00
#define P2_NO_CHAINCODE 0x00
#define P2_CHAINCODE 0x01
#define P1_FIRST 0x00
#define P1_MORE 0x80

#define COMMON_CLA 0xB0
#define COMMON_INS_GET_WALLET_ID 0x04

#define OFFSET_CLA 0
#define OFFSET_INS 1
#define OFFSET_P1 2
#define OFFSET_P2 3
#define OFFSET_LC 4
#define OFFSET_CDATA 5

static const uint8_t TOKEN_SIGNATURE_PUBLIC_KEY[] = {
// cLabs production key (2020-04-14)
  0x04,

  0xb0,0x6c,0xf5,0xd8,0xf7,0xed,0x71,0xd8,
  0xbd,0x9b,0x9d,0xc3,0x79,0x44,0xa1,0xc6,
  0xd2,0x40,0xf6,0x9b,0xb0,0xbe,0x36,0x21,
  0xdd,0xdb,0xb6,0xac,0xe,0xcc,0xd1,0x50,

  0x8b,0xcc,0x2e,0xa4,0x62,0x27,0xe4,0x3b,
  0x94,0x1e,0x2c,0x6f,0x1b,0x1c,0xd0,0xae,
  0x68,0xe5,0x4b,0x18,0x5e,0x2c,0xab,0xef,
  0x34,0x55,0x58,0x6,0x4,0xbd,0x45,0xb8
};

dataContext_t dataContext;

tmpCtx_t tmpCtx;

txContext_t txContext;

tmpContent_t tmpContent;

cx_sha3_t sha3;

volatile uint8_t dataAllowed;
volatile uint8_t contractDetails;
volatile bool dataPresent;
volatile bool tokenProvisioned;

strings_t strings;

const internalStorage_t N_storage_real;

static const char SIGN_MAGIC[] = "\x19"
                                 "Ethereum Signed Message:\n";

chain_config_t *chainConfig;

unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len) {
    switch (channel & ~(IO_FLAGS)) {
    case CHANNEL_KEYBOARD:
        break;

    // multiplexed io exchange over a SPI channel and TLV encapsulated protocol
    case CHANNEL_SPI:
        if (tx_len) {
            io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);

            if (channel & IO_RESET_AFTER_REPLIED) {
                reset();
            }
            return 0; // nothing received from the master so far (it's a tx
                      // transaction)
        } else {
            return io_seproxyhal_spi_recv(G_io_apdu_buffer,
                                          sizeof(G_io_apdu_buffer), 0);
        }

    default:
        THROW(INVALID_PARAMETER);
    }
    return 0;
}

unsigned int const U_os_perso_seed_cookie[] = {
  0xda7aba5e,
  0xc1a551c5,
};

#ifndef HAVE_WALLET_ID_SDK

void handleGetWalletId(volatile unsigned int *tx) {
  unsigned char t[64];
  cx_ecfp_256_private_key_t priv;
  cx_ecfp_256_public_key_t pub;
  // seed => priv key
  os_perso_derive_node_bip32(CX_CURVE_256K1, U_os_perso_seed_cookie, 2, t, NULL);
  // priv key => pubkey
  cx_ecdsa_init_private_key(CX_CURVE_256K1, t, 32, &priv);
  cx_ecfp_generate_pair(CX_CURVE_256K1, &pub, &priv, 1);
  // pubkey -> sha512
  cx_hash_sha512(pub.W, sizeof(pub.W), t, sizeof(t));
  // ! cookie !
  memcpy(G_io_apdu_buffer, t, 64);  
  *tx = 64;
  THROW(0x9000);
}

#endif

void handleGetPublicKey(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
  UNUSED(dataLength);
  uint8_t privateKeyData[32];
  uint32_t bip32Path[MAX_BIP32_PATH];
  uint32_t i;
  uint8_t bip32PathLength = *(dataBuffer++);
  cx_ecfp_private_key_t privateKey;
  reset_app_context();
  if ((bip32PathLength < 0x01) ||
      (bip32PathLength > MAX_BIP32_PATH)) {
    PRINTF("Invalid path\n");
    THROW(0x6a80);
  }
  if ((p1 != P1_CONFIRM) && (p1 != P1_NON_CONFIRM)) {
    THROW(0x6B00);
  }
  if ((p2 != P2_CHAINCODE) && (p2 != P2_NO_CHAINCODE)) {
    THROW(0x6B00);
  }
  for (i = 0; i < bip32PathLength; i++) {
    bip32Path[i] = U4BE(dataBuffer, 0);
    dataBuffer += 4;
  }
  tmpCtx.publicKeyContext.getChaincode = (p2 == P2_CHAINCODE);
  io_seproxyhal_io_heartbeat();
  os_perso_derive_node_bip32(CX_CURVE_256K1, bip32Path, bip32PathLength, privateKeyData, (tmpCtx.publicKeyContext.getChaincode ? tmpCtx.publicKeyContext.chainCode : NULL));
  cx_ecfp_init_private_key(CX_CURVE_256K1, privateKeyData, 32, &privateKey);
  io_seproxyhal_io_heartbeat();
  cx_ecfp_generate_pair(CX_CURVE_256K1, &tmpCtx.publicKeyContext.publicKey, &privateKey, 1);
  explicit_bzero(&privateKey, sizeof(privateKey));
  explicit_bzero(privateKeyData, sizeof(privateKeyData));
  io_seproxyhal_io_heartbeat();
  getEthAddressStringFromKey(&tmpCtx.publicKeyContext.publicKey, tmpCtx.publicKeyContext.address, &sha3);
#ifndef NO_CONSENT
  if (p1 == P1_NON_CONFIRM)
#endif // NO_CONSENT
  {
    *tx = set_result_get_publicKey();
    THROW(0x9000);
  }
#ifndef NO_CONSENT
  else {
    // prepare for a UI based reply
    snprintf(strings.common.fullAddress, sizeof(strings.common.fullAddress), "0x%.*s", 40, tmpCtx.publicKeyContext.address);
    ux_flow_init(0, ux_display_public_flow, NULL);
    *flags |= IO_ASYNCH_REPLY;
  }
#endif // NO_CONSENT
}

void handleProvideErc20TokenInformation(uint8_t p1, uint8_t p2, uint8_t *workBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
  UNUSED(p1);
  UNUSED(p2);
  UNUSED(flags);
  UNUSED(tx);
  uint32_t offset = 0;
  uint8_t tickerLength;
  uint32_t chainId;
  uint8_t hash[32];
  cx_ecfp_public_key_t tokenKey;


  tmpCtx.transactionContext.currentTokenIndex = (tmpCtx.transactionContext.currentTokenIndex + 1) % MAX_TOKEN;
  tokenDefinition_t* token = &tmpCtx.transactionContext.tokens[tmpCtx.transactionContext.currentTokenIndex];

  PRINTF("Provisioning currentTokenIndex %d\n", tmpCtx.transactionContext.currentTokenIndex);

  if (dataLength < 1) {
    THROW(0x6A80);
  }
  tickerLength = workBuffer[offset++];
  dataLength--;
  // We need to make sure we can write the ticker, a space and a zero byte at the end
  if ((tickerLength + 2) >= sizeof(token->ticker)) {
    THROW(0x6A80);
  }
  if (dataLength < tickerLength + 20 + 4 + 4) {
    THROW(0x6A80);
  }
  cx_hash_sha256(workBuffer + offset, tickerLength + 20 + 4 + 4, hash, 32);
  memcpy(token->ticker, workBuffer + offset, tickerLength);
  token->ticker[tickerLength] = ' ';
  token->ticker[tickerLength + 1] = '\0';
  offset += tickerLength;
  dataLength -= tickerLength;
  memcpy(token->address, workBuffer + offset, 20);
  offset += 20;
  dataLength -= 20;
  token->decimals = U4BE(workBuffer, offset);
  offset += 4;
  dataLength -= 4;
  chainId = U4BE(workBuffer, offset);
  if ((chainConfig->chainId != 0) && (chainConfig->chainId != chainId)) {
    PRINTF("ChainId token mismatch\n");
    THROW(0x6A80);
  }
  offset += 4;
  dataLength -= 4;
  cx_ecfp_init_public_key(CX_CURVE_256K1, TOKEN_SIGNATURE_PUBLIC_KEY, sizeof(TOKEN_SIGNATURE_PUBLIC_KEY), &tokenKey);
  if (!cx_ecdsa_verify(&tokenKey, CX_LAST, CX_SHA256, hash, 32, workBuffer + offset, dataLength)) {
    PRINTF("Invalid token signature\n");
    THROW(0x6A80);
  }
  tmpCtx.transactionContext.tokenSet[tmpCtx.transactionContext.currentTokenIndex] = 1;
  THROW(0x9000);
}

void handleSign(uint8_t p1, uint8_t p2, uint8_t *workBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
  UNUSED(tx);
  parserStatus_e txResult;
  if (p1 == P1_FIRST) {
    if (dataLength < 1) {
      PRINTF("Invalid data\n");
      THROW(0x6a80);
    }    
    if (appState != APP_STATE_IDLE) {
      reset_app_context();
    }
    appState = APP_STATE_SIGNING_TX;    
    tmpCtx.transactionContext.pathLength = workBuffer[0];
    if ((tmpCtx.transactionContext.pathLength < 0x01) ||
        (tmpCtx.transactionContext.pathLength > MAX_BIP32_PATH)) {
      PRINTF("Invalid path\n");
      THROW(0x6a80);
    }
    workBuffer++;
    dataLength--;
    for (int i = 0; i < tmpCtx.transactionContext.pathLength; i++) {
      if (dataLength < 4) {
        PRINTF("Invalid data\n");
        THROW(0x6a80);
      }      
      tmpCtx.transactionContext.bip32Path[i] = U4BE(workBuffer, 0);
      workBuffer += 4;
      dataLength -= 4;
    }
    dataPresent = false;
    tokenProvisioned = false;
    initTx(&txContext, &sha3, &tmpContent.txContent, customProcessor, NULL);
  }
  else
  if (p1 != P1_MORE) {
    THROW(0x6B00);
  }
  if (p2 != 0) {
    THROW(0x6B00);
  }
  if ((p1 == P1_MORE) && (appState != APP_STATE_SIGNING_TX)) {
    PRINTF("Signature not initialized\n");
    THROW(0x6985);
  }
  if (txContext.currentField == TX_RLP_NONE) {
    PRINTF("Parser not initialized\n");
    THROW(0x6985);
  }
  txResult = processTx(&txContext, workBuffer, dataLength, (chainConfig->kind == CHAIN_KIND_WANCHAIN ? TX_FLAG_TYPE : 0));
  switch (txResult) {
    case USTREAM_SUSPENDED:
      break;
    case USTREAM_FINISHED:
      break;
    case USTREAM_PROCESSING:
      THROW(0x9000);
    case USTREAM_FAULT:
      THROW(0x6A80);
    default:
      PRINTF("Unexpected parser status\n");
      THROW(0x6A80);
  }

  *flags |= IO_ASYNCH_REPLY;

  if (txResult == USTREAM_FINISHED) {
    finalizeParsing(true);
  }
}

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
  G_io_apdu_buffer[1] = LEDGER_MAJOR_VERSION;
  G_io_apdu_buffer[2] = LEDGER_MINOR_VERSION;
  G_io_apdu_buffer[3] = LEDGER_PATCH_VERSION;
  *tx = 4;
  THROW(0x9000);
}

void handleGetAppType(uint8_t p1, uint8_t p2, uint8_t *workBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
  UNUSED(p1);
  UNUSED(p2);
  UNUSED(workBuffer);
  UNUSED(dataLength);
  UNUSED(flags);
  G_io_apdu_buffer[0] = APP_TYPE;
  *tx = 1;
  THROW(0x9000);
}
			

void handleSignPersonalMessage(uint8_t p1, uint8_t p2, uint8_t *workBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
  UNUSED(tx);

  if (p1 == P1_FIRST) {
    char tmp[11];
    uint32_t index;
    uint32_t base = 10;
    uint8_t pos = 0;
    uint32_t i;
    if (dataLength < 1) {
      PRINTF("Invalid data\n");
      THROW(0x6a80);
    }
    if (appState != APP_STATE_IDLE) {
      reset_app_context();
    }
    appState = APP_STATE_SIGNING_MESSAGE;    
    tmpCtx.messageSigningContext.pathLength = workBuffer[0];
    if ((tmpCtx.messageSigningContext.pathLength < 0x01) ||
        (tmpCtx.messageSigningContext.pathLength > MAX_BIP32_PATH)) {
        PRINTF("Invalid path\n");
        THROW(0x6a80);
    }
    workBuffer++;
    dataLength--;
    for (i = 0; i < tmpCtx.messageSigningContext.pathLength; i++) {
        if (dataLength < 4) {
          PRINTF("Invalid data\n");
          THROW(0x6a80);
        }
        tmpCtx.messageSigningContext.bip32Path[i] = U4BE(workBuffer, 0);
        workBuffer += 4;
        dataLength -= 4;
    }
    if (dataLength < 4) {
      PRINTF("Invalid data\n");
      THROW(0x6a80);
    }    
    tmpCtx.messageSigningContext.remainingLength = U4BE(workBuffer, 0);
    workBuffer += 4;
    dataLength -= 4;
    // Initialize message header + length
    cx_keccak_init(&sha3, 256);
    cx_hash((cx_hash_t *)&sha3, 0, (uint8_t*)SIGN_MAGIC, sizeof(SIGN_MAGIC) - 1, NULL, 0);
    for (index = 1; (((index * base) <= tmpCtx.messageSigningContext.remainingLength) &&
                         (((index * base) / base) == index));
             index *= base);
    for (; index; index /= base) {
      tmp[pos++] = '0' + ((tmpCtx.messageSigningContext.remainingLength / index) % base);
    }
    tmp[pos] = '\0';
    cx_hash((cx_hash_t *)&sha3, 0, (uint8_t*)tmp, pos, NULL, 0);
    cx_sha256_init(&tmpContent.sha2);
  }
  else if (p1 != P1_MORE) {
    THROW(0x6B00);
  }
  if (p2 != 0) {
    THROW(0x6B00);
  }
  if ((p1 == P1_MORE) && (appState != APP_STATE_SIGNING_MESSAGE)) {
    PRINTF("Signature not initialized\n");
    THROW(0x6985);
  }
  if (dataLength > tmpCtx.messageSigningContext.remainingLength) {
      THROW(0x6A80);
  }
  cx_hash((cx_hash_t *)&sha3, 0, workBuffer, dataLength, NULL, 0);
  cx_hash((cx_hash_t *)&tmpContent.sha2, 0, workBuffer, dataLength, NULL, 0);
  tmpCtx.messageSigningContext.remainingLength -= dataLength;
  if (tmpCtx.messageSigningContext.remainingLength == 0) {
    uint8_t hashMessage[32];

    cx_hash((cx_hash_t *)&sha3, CX_LAST, workBuffer, 0, tmpCtx.messageSigningContext.hash, 32);
    cx_hash((cx_hash_t *)&tmpContent.sha2, CX_LAST, workBuffer, 0, hashMessage, 32);

#define HASH_LENGTH 4
    array_hexstr(strings.common.fullAddress, hashMessage, HASH_LENGTH / 2);
    strings.common.fullAddress[HASH_LENGTH / 2 * 2] = '.';
    strings.common.fullAddress[HASH_LENGTH / 2 * 2 + 1] = '.';
    strings.common.fullAddress[HASH_LENGTH / 2 * 2 + 2] = '.';
    array_hexstr(strings.common.fullAddress + HASH_LENGTH / 2 * 2 + 3, hashMessage + 32 - HASH_LENGTH / 2, HASH_LENGTH / 2);

#ifdef NO_CONSENT
    io_seproxyhal_touch_signMessage_ok(NULL);
#else
    ux_flow_init(0, ux_sign_flow, NULL);
#endif // NO_CONSENT

    *flags |= IO_ASYNCH_REPLY;

  } else {
    THROW(0x9000);
  }
}

void handleApdu(volatile unsigned int *flags, volatile unsigned int *tx) {
  unsigned short sw = 0;

  BEGIN_TRY {
    TRY {

#ifndef HAVE_WALLET_ID_SDK

      if ((G_io_apdu_buffer[OFFSET_CLA] == COMMON_CLA) && (G_io_apdu_buffer[OFFSET_INS] == COMMON_INS_GET_WALLET_ID)) {
        handleGetWalletId(tx);
        return;
      }

#endif

      if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
        THROW(0x6E00);
      }

      switch (G_io_apdu_buffer[OFFSET_INS]) {
        case INS_GET_PUBLIC_KEY:
          memset(tmpCtx.transactionContext.tokenSet, 0, MAX_TOKEN);
          handleGetPublicKey(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
          break;

        case INS_PROVIDE_ERC20_TOKEN_INFORMATION:
          handleProvideErc20TokenInformation(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
          break;

        case INS_SIGN:
          handleSign(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
          break;

        case INS_GET_APP_CONFIGURATION:
          handleGetAppConfiguration(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
          break;

        case INS_SIGN_PERSONAL_MESSAGE:
          memset(tmpCtx.transactionContext.tokenSet, 0, MAX_TOKEN);
          handleSignPersonalMessage(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
          break;

	case INS_GET_APP_TYPE:
	  handleGetAppType(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
	  break;

#if 0
        case 0xFF: // return to dashboard
          goto return_to_dashboard;
#endif

        default:
          THROW(0x6D00);
          break;
      }
    }
    CATCH(EXCEPTION_IO_RESET) {
      THROW(EXCEPTION_IO_RESET);
    }
    CATCH_OTHER(e) {
      switch (e & 0xF000) {
        case 0x6000:
          // Wipe the transaction context and report the exception
          sw = e;
          reset_app_context();
          break;
        case 0x9000:
          // All is well
          sw = e;
          break;
        default:
          // Internal error
          sw = 0x6800 | (e & 0x7FF);
          reset_app_context();
          break;
        }
        if (e != 0x9000) {
          *flags &= ~IO_ASYNCH_REPLY;
        }
        // Unexpected exception => report
        G_io_apdu_buffer[*tx] = sw >> 8;
        G_io_apdu_buffer[*tx + 1] = sw;
        *tx += 2;
      }
      FINALLY {
      }
  }
  END_TRY;
}

void sample_main(void) {
    volatile unsigned int tx = 0;
    volatile unsigned int flags = 0;

    // DESIGN NOTE: the bootloader ignores the way APDU are fetched. The only
    // goal is to retrieve APDU.
    // When APDU are to be fetched from multiple IOs, like NFC+USB+BLE, make
    // sure the io_event is called with a
    // switch event, before the apdu is replied to the bootloader. This avoid
    // APDU injection faults.
    for (;;) {
        volatile unsigned int rx = 0;
        volatile unsigned short sw = 0;

        BEGIN_TRY {
            TRY {
                rx = tx;
                tx = 0; // ensure no race in catch_other if io_exchange throws
                        // an error
                rx = io_exchange(CHANNEL_APDU | flags, rx);
                flags = 0;

                // no apdu received, well, reset the session, and reset the
                // bootloader configuration
                if (rx == 0) {
                    THROW(0x6982);
                }

                PRINTF("New APDU received:\n%.*H\n", rx, G_io_apdu_buffer);

                handleApdu(&flags, &tx);
            }
            CATCH(EXCEPTION_IO_RESET) {
              THROW(EXCEPTION_IO_RESET);
            }
            CATCH_OTHER(e) {
                switch (e & 0xF000) {
                case 0x6000:
                    // Wipe the transaction context and report the exception
                    sw = e;
                    reset_app_context();
                    break;
                case 0x9000:
                    // All is well
                    sw = e;
                    break;
                default:
                    // Internal error
                    sw = 0x6800 | (e & 0x7FF);
                    reset_app_context();
                    break;
                }
                if (e != 0x9000) {
                    flags &= ~IO_ASYNCH_REPLY;
                }
                // Unexpected exception => report
                G_io_apdu_buffer[tx] = sw >> 8;
                G_io_apdu_buffer[tx + 1] = sw;
                tx += 2;
            }
            FINALLY {
            }
        }
        END_TRY;
    }

//return_to_dashboard:
    return;
}

// override point, but nothing more to do
void io_seproxyhal_display(const bagl_element_t *element) {
  io_seproxyhal_display_default((bagl_element_t *)element);
}

unsigned char io_event(unsigned char channel) {
    UNUSED(channel);

    // nothing done with the event, throw an error on the transport layer if
    // needed

    // can't have more than one tag in the reply, not supported yet.
    switch (G_io_seproxyhal_spi_buffer[0]) {
    case SEPROXYHAL_TAG_FINGER_EVENT:
    		UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
    		break;

    case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
        UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
        break;

    case SEPROXYHAL_TAG_STATUS_EVENT:
        if (G_io_apdu_media == IO_APDU_MEDIA_USB_HID && !(U4BE(G_io_seproxyhal_spi_buffer, 3) & SEPROXYHAL_TAG_STATUS_EVENT_FLAG_USB_POWERED)) {
         THROW(EXCEPTION_IO_RESET);
        }
        // no break is intentional
    default:
        UX_DEFAULT_EVENT();
        break;

    case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
        UX_DISPLAYED_EVENT({});
        break;

    case SEPROXYHAL_TAG_TICKER_EVENT:
        UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {});
        break;
    }

    // close the event if not done previously (by a display or whatever)
    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }

    // command has been processed, DO NOT reset the current APDU transport
    return 1;
}

void app_exit(void) {

    BEGIN_TRY_L(exit) {
        TRY_L(exit) {
            os_sched_exit(-1);
        }
        FINALLY_L(exit) {

        }
    }
    END_TRY_L(exit);
}

chain_config_t const C_chain_config = {
  .coinName = CHAINID_COINNAME " ",
  .chainId = CHAIN_ID,
  .kind = CHAIN_KIND,
};

__attribute__((section(".boot"))) int main(int arg0) {
#ifdef USE_LIB_ETHEREUM
    chain_config_t local_chainConfig;
    memcpy(&local_chainConfig, &C_chain_config, sizeof(chain_config_t));
    unsigned int libcall_params[3];
    unsigned char coinName[sizeof(CHAINID_COINNAME)];
    strcpy(coinName, CHAINID_COINNAME);
    local_chainConfig.coinName = coinName;
    BEGIN_TRY {
        TRY {
            // ensure syscall will accept us
            check_api_level(CX_COMPAT_APILEVEL);
            // delegate to Ethereum app/lib
            libcall_params[0] = "Ethereum";
            libcall_params[1] = 0x100; // use the Init call, as we won't exit
            libcall_params[2] = &local_chainConfig;
            os_lib_call(&libcall_params);
        }
        FINALLY {
            app_exit();
        }
    }
    END_TRY;
#else
    // exit critical section
    __asm volatile("cpsie i");

    if (arg0) {
        if (((unsigned int *)arg0)[0] != 0x100) {
            app_exit();
            return 0;
        }
        chainConfig = (chain_config_t *)((unsigned int *)arg0)[1];
    }
    else {
        chainConfig = (chain_config_t *)PIC(&C_chain_config);
    }

    reset_app_context();
    tmpCtx.transactionContext.currentTokenIndex = 0;

    // ensure exception will work as planned
    os_boot();

    for (;;) {
        UX_INIT();

        BEGIN_TRY {
            TRY {
                io_seproxyhal_init();

#ifdef TARGET_NANOX
                // grab the current plane mode setting
                G_io_app.plane_mode = os_setting_get(OS_SETTING_PLANEMODE, NULL, 0);
#endif // TARGET_NANOX

                if (N_storage.initialized != 0x01) {
                  internalStorage_t storage;
                  storage.dataAllowed = 0x01;
                  storage.contractDetails = 0x00;
                  storage.initialized = 0x01;
                  nvm_write(&N_storage, (void*)&storage, sizeof(internalStorage_t));
                }
                dataAllowed = N_storage.dataAllowed;
                contractDetails = N_storage.contractDetails;

                USB_power(0);
                USB_power(1);

#ifdef HAVE_BLE
                // BLE has to be powered on before ui_idle() call for Blue devices only
                BLE_power(0, NULL);
                BLE_power(1, "Nano X");
#endif // HAVE_BLE
                
                ui_idle();
                sample_main();
            }
            CATCH(EXCEPTION_IO_RESET) {
              // reset IO and UX before continuing
              continue;
            }
            CATCH_ALL {
              break;
            }
            FINALLY {
            }
        }
        END_TRY;
    }
	  app_exit();
#endif
    return 0;
}
