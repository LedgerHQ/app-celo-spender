#include "get_wallet_id.h"

#include "io.h"
#include "sw.h"
#include "os.h"
#include "globals.h"
#include "constants.h"
#include "celo.h"
#include "utils.h"
#include "ethUtils.h"
#include "ui_common.h"

#ifndef HAVE_WALLET_ID_SDK

unsigned int const U_os_perso_seed_cookie[] = {
  0xda7aba5e,
  0xc1a551c5,
};

/**
 * Handle Get Wallet ID command
 *
 * @param tx The transaction buffer
 */
void handleGetWalletId(volatile unsigned int *tx) {
  unsigned char t[64];
  cx_ecfp_256_private_key_t priv;
  cx_ecfp_256_public_key_t pub;
  // seed => priv key
  CX_THROW(os_derive_bip32_no_throw(CX_CURVE_256K1, U_os_perso_seed_cookie, 2, t, NULL));
  // priv key => pubkey
  CX_THROW(cx_ecdsa_init_private_key(CX_CURVE_256K1, t, 32, &priv));
  CX_THROW(cx_ecfp_generate_pair_no_throw(CX_CURVE_256K1, &pub, &priv, 1));
  // pubkey -> sha512
  cx_hash_sha512(pub.W, sizeof(pub.W), t, sizeof(t));
  // ! cookie !
  memcpy(G_io_apdu_buffer, t, 64);
  *tx = 64;
  THROW(SW_OK);
}

#endif // HAVE_WALLET_ID_SDK
