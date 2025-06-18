#pragma once

// App flags
#define APP_FLAG_DATA_ALLOWED          0x01
#define APP_FLAG_EXTERNAL_TOKEN_NEEDED 0x02

// App type
#define APP_TYPE 0x02

// APDU instructions
#define CLA                                 0xE0
#define INS_GET_PUBLIC_KEY                  0x02
#define INS_SIGN                            0x04
#define INS_GET_APP_CONFIGURATION           0x06
#define INS_SIGN_PERSONAL_MESSAGE           0x08
#define INS_PROVIDE_ERC20_TOKEN_INFORMATION 0x0A
#define INS_GET_APP_TYPE                    0x0C

// Common instructions
#define COMMON_CLA               0xB0
#define COMMON_INS_GET_WALLET_ID 0x04

// P1 and P2 values
#define P1_CONFIRM      0x01
#define P1_NON_CONFIRM  0x00
#define P2_NO_CHAINCODE 0x00
#define P2_CHAINCODE    0x01
#define P1_FIRST        0x00
#define P1_MORE         0x80

// Constants
#define MAX_BIP32_PATH 10
#define MAX_TOKEN      2
#define SIGNATURE_LEN  65

// Public key length
#define PUBKEY_LEN    65
#define CHAINCODE_LEN 32