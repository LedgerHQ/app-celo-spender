from ragger.bip import pack_derivation_path

CELO_DERIVATION_PATH = "m/44'/52752'/12345'"
ETH_DERIVATION_PATH = "m/44'/60'/0'"

CELO_PACKED_DERIVATION_PATH = pack_derivation_path("m/44'/52752'/12345'")
ETH_PACKED_DERIVATION_PATH = pack_derivation_path("m/44'/60'/0'")
