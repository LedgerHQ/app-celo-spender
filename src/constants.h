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
#define INS_SIGN_EIP_712_MESSAGE            0x0C
#define INS_EIP712_STRUCT_DEF               0x1A
#define INS_EIP712_STRUCT_IMPL              0x1C
#define INS_EIP712_FILTERING                0x1E
#define INS_GET_APP_TYPE                    0xFF

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
// P1 AND P2 FROM ETH
#define P1_FIRST_CHUNK          0x01
#define P1_FOLLOWING_CHUNK      0x00
#define P2_EIP712_LEGACY_IMPLEM 0x00
#define P2_EIP712_FULL_IMPLEM   0x01

// Constants
#define MAX_BIP32_PATH 10
#define SIGNATURE_LEN  65
#define HASH_LENGTH    32
// Public key length
#define PUBKEY_LEN    65
#define ADDRESS_LEN   40
#define CHAINCODE_LEN 32

// The byte size of a Keccak-256 hash.
#define KECCAK256_HASH_BYTESIZE 32
// The standard length of an Ethereum address
#define ADDRESS_LENGTH 20

// The length of a 256-bit integer in bytes.
#define INT256_LENGTH 32

#define MAX_TOKENS 5

#define ADDRESS_LENGTH     20
#define INT256_LENGTH      32
#define MAX_TICKER_LEN     11  // 10 characters + '\0'
#define CX_KECCAK_256_SIZE 32
