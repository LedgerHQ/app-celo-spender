// #include "os_utils.h"
// #include "os_pic.h"
// #include "globals.h"
// #include "utils.h"
// #include "ethUstream.h"
// #include "network.h"
// // #include "network_info.h"
// // #include "shared_context.h"
// // #include "common_utils.h"
// // #include "apdu_constants.h"

// const char g_unknown_ticker[] = "???";

// // Mapping of chain ids to networks.
// static const network_info_t NETWORK_MAPPING[] = {
//     {.chain_id = 42220, .name = "Celo", .ticker = "CELO"},
//     {.chain_id = 44787, .name = "Celo Alfajores", .ticker = "aCELO"},
// };

// static const network_info_t *get_network_from_chain_id(const uint64_t *chain_id) {
//     if (*chain_id != 0) {
//         // Look if the network is available
//         for (size_t i = 0; i < MAX_DYNAMIC_NETWORKS; i++) {
//             if (DYNAMIC_NETWORK_INFO[i].chain_id == *chain_id) {
//                 PRINTF("[NETWORK] - Found dynamic \"%s\" in slot %u\n",
//                        DYNAMIC_NETWORK_INFO[i].name,
//                        i);
//                 return (const network_info_t *) &DYNAMIC_NETWORK_INFO[i];
//             }
//         }

//         // Fallback to hardcoded table
//         for (size_t i = 0; i < ARRAYLEN(NETWORK_MAPPING); i++) {
//             if (NETWORK_MAPPING[i].chain_id == *chain_id) {
//                 PRINTF("[NETWORK] - Fallback on hardcoded list. Found %s\n",
//                        NETWORK_MAPPING[i].name);
//                 return (const network_info_t *) &NETWORK_MAPPING[i];
//             }
//         }
//     }
//     return NULL;
// }

// const char *get_network_name_from_chain_id(const uint64_t *chain_id) {
//     const network_info_t *net = get_network_from_chain_id(chain_id);

//     if (net == NULL) {
//         return NULL;
//     }
//     return PIC(net->name);
// }

// uint16_t get_network_as_string(char *out, size_t out_size) {
//     uint64_t chain_id = get_tx_chain_id();
//     const char *name = get_network_name_from_chain_id(&chain_id);

//     if (name == NULL) {
//         // No network name found so simply copy the chain ID as the network name.
//         if (!u64_to_string(chain_id, out, out_size)) {
//             return APDU_RESPONSE_CHAINID_OUT_BUF_SMALL;
//         }
//     } else {
//         // Network name found, simply copy it.
//         strlcpy(out, name, out_size);
//     }
//     return APDU_RESPONSE_OK;
// }

// const char *get_network_ticker_from_chain_id(const uint64_t *chain_id) {
//     const network_info_t *net = get_network_from_chain_id(chain_id);

//     if (net == NULL) {
//         return NULL;
//     }
//     return PIC(net->ticker);
// }

// bool chain_is_ethereum_compatible(const uint64_t *chain_id) {
//     return get_network_from_chain_id(chain_id) != NULL;
// }

// // Returns the chain ID. Defaults to 0 if txType was not found (For TX).
// uint64_t get_tx_chain_id(void) {
//     uint64_t chain_id = 0;

//     switch (txContext.txType) {
//         case CIP64:
//         // km: is the chain id at the same place for CIP 64 and EIP1159
//         case EIP1559:
//             chain_id = u64_from_BE(tmpContent.txContent.chainID.value,
//                                    tmpContent.txContent.chainID.length);
//             break;
//         default:
//             PRINTF("Txtype `%d` not supported while generating chainID\n", txContext.txType);
//             break;
//     }
//     return chain_id;
// }

// const char *get_displayable_ticker(const uint64_t *chain_id, const chain_config_t *chain_cfg) {
//     const char *ticker = get_network_ticker_from_chain_id(chain_id);

//     if (ticker == NULL) {
//         if (*chain_id == chain_cfg->chainId) {
//             ticker = chain_cfg->coinName;
//         } else {
//             ticker = g_unknown_ticker;
//         }
//     }
//     return ticker;
// }

// /**
//  * Checks whether the app can support the given chain ID
//  *
//  * - If the given chain ID is the same as the app's one
//  * - If both chain IDs are present in the array of Ethereum-compatible networks
//  */
// bool app_compatible_with_chain_id(const uint64_t *chain_id) {
//     return ((chainConfig->chainId == *chain_id) ||
//             (chain_is_ethereum_compatible(&chainConfig->chainId) &&
//              chain_is_ethereum_compatible(chain_id)));
// }
