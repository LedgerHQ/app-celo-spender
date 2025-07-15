import struct
from typing import List, Generator, Optional
from enum import IntEnum
from contextlib import contextmanager

from ledgered.devices import DeviceType


from ragger.error import ExceptionRAPDU
from ragger.backend.interface import BackendInterface, RAPDU

import rlp
from ragger.bip import pack_derivation_path

# ei712 imports
from .celo_command_builder import CommandBuilder
from .eip712 import EIP712FieldType
from .eth_keychain import sign_data, Key
from .eth_tlv import format_tlv, FieldTag
from .eth_response_parser import pk_addr
from .eth_tx_simu import TxSimu


# Token signature mapping based on ticker and chain ID
TOKEN_SIGNATURES = {
    # Celo Mainnet (42220)
    (
        "USDT",
        42220,
    ): "30450220270f3b05aa358663314cff7f135f0c318f5300fee2f8b87737b73d477fd264b50221009c12f0444493542f55fec16be2edd144afdbdcef6e090307615c4c9d8a52b068",
    (
        "USDC",
        42220,
    ): "3046022100c986d7d103dcf0e433e0835ad88edbdeb65d0e1488ec68d0d4ebf6f10a658c3e0221009f5f28e7944d803e00db65aae14bcf1dea40e2fb7b1b65526b0caeea03309b25",
    (
        "eXOF",
        42220,
    ): "3044022071c45a6cdcc2a07ef4b3adb9b92511534f5d65d84a7ff13525bfe1c3bce4e8e002200d83191e24f27d08d1d5b8521a4af6ab54d995b7fc7411f0cbaf205e781008eb",
    (
        "cUSD",
        42220,
    ): "304402201f8217a6310b78bfe63f40e767b1413e7480403603486f8d3b2cfd4aaa8607470220577c04cc84d1e0ad9d344c73b624e0fc36c63d4a5f33654ab0bcd892b068e7a1",
    (
        "cEUR",
        42220,
    ): "30460221008cd66dd4d1ecbe8c02744864d78dd2702bfeaa6fb530b1118cdb3e7fe34976d3022100ce870eeda25a52819d1515b9634db0aae7a9551c7619edebe709a4ec729415ed",
    (
        "cREAL",
        42220,
    ): "3046022100cbc2651a555bee21140187fe819f595d704d3a16662c7fa0016c3c900419d29f022100dc14b3f458e4476f1d82991059a6e08ceeb95f808fd30a9bfe25da84bd0b31d5",
    # Baklava Testnet (62320)
    (
        "b cUSD",
        62320,
    ): "304402200f64d7b8cdef156dc9b4af61df804b4fb0fdcc562f613e386894baa7c52879db02207998a030e8ed6ba4cb3f1796c3731016d605b507e8b1c8df3aa904d8fe7d2149",
    (
        "b eXOF",
        62320,
    ): "3045022100a2e46dc991fa23c8b30dd45e3cd390b1d5f6968620d1eb9a1fcd97452ef1d1b3022060eaac9d0196506997407c3098507366f3da628b8352b6cf0f0b4157665be116",
    (
        "b cREAL",
        62320,
    ): "304502210094ac6ee44facb616394205ce2225a4da505f2579a9ba97ae1f6dc5f45962664e02204802306307fa3c1b9bc25ee7cbb5af2bc39ac828ce9e640a989ff2206737bdbb",
    (
        "b cEUR",
        62320,
    ): "304402204f3ac27a518b05e1ff41a3c8a0ae3f448e70219c77e29c68a918d9aa4cbe7bb02200b59b74c7b17e2513c72d12b10dc451656ebc9b3511a8f7b481963e04db5ca7b",
    # Alfajores Testnet (44787)
    (
        "a G$",
        44787,
    ): "3045022100df82dda7a93ab24204e0c589362b5eb0c229c2f67cd638309bbeeca1b1d084bd0220337c946db2328d8cb27e62fb0b12f0a2fc507a163b10614ce77e791550c898b6",
    (
        "a cEUR",
        44787,
    ): "304402202db5555d4efbdabdc49c3614ca3b35bf57c0d44e5e754ceff24cde0bafb50361022010271299c97cc94518f27f32718c72b0385b61f4773383b1cbf8217007ca15bb",
    (
        "a EFC",
        44787,
    ): "3045022100f4f245e8c0d442e37470e0ee3092876062272256b6272ee648916086d098447002205ce9fe66a286a3c2c3c9c8f3bc93aa44e1394338745ccc0878990da6cc05ad54",
    (
        "a USDC",
        44787,
    ): "3045022100e97ac2814272a9823cf94c6e8c813eae72ed95484018befda82c93ea22f28f1d022059aad1a356c57a453750ca7a64befeda2100693c2d53e6f5e5e7c07eab098965",
    (
        "a cUSD",
        44787,
    ): "3045022100946800b02fb008f5d714a460e087e087c730d5ddec3182456c546352e1334a6c0220174c8a33d3e043a0e510f2bf8c28826d1be5ddd0dfd019ec87c57341bee27561",
    (
        "a eXOF",
        44787,
    ): "3044022021bc9b34a64672beffcd42d1c9b7168f1d4a8a65a959816d008f81b36e410eeb022024107b93507fbdf9afc03b296df622423bef267fb554a230ead0c621acb9d110",
    (
        "a USDT",
        44787,
    ): "304502200d678ca7f9db0a8b3062d939050fab13f96d7b6a382b59b4f1e526f0db155b6c022100bf40af627b637c0310b7c30c41c8d81e24e64c97b04ac4d9561ce54ecdb48c84",
    (
        "a cREAL",
        44787,
    ): "3046022100a9705c551606208be1ecd9f186eeef966fd3b3d669fcc80f367c1068c17ca4ab022100fe65dae3312312f71ea5a83dfc538d100d33d651337d47df8ecfc28e8ffe4af9",
}


class INS(IntEnum):
    INS_GET_PUBLIC_KEY = 0x02
    INS_SIGN = 0x04
    INS_GET_APP_CONFIGURATION = 0x06
    INS_SIGN_PERSONAL_MESSAGE = 0x08
    INS_PROVIDE_ERC20_TOKEN_INFORMATION = 0x0A
    INS_GET_APP_TYPE = 0xFF


CLA = 0xE0

MAX_CHUNK_SIZE = 255


class StatusCode(IntEnum):
    STATUS_OK = 0x9000
    STATUS_DEPRECATED = 0x6501
    STATUS_ERROR_IN_DATA = 0x6A80
    STATUS_NOT_IMPLEMENTED = 0x911C


class TrustedNameType(IntEnum):
    ACCOUNT = 0x01
    CONTRACT = 0x02
    NFT = 0x03


class TrustedNameSource(IntEnum):
    LAB = 0x00
    CAL = 0x01
    ENS = 0x02
    UD = 0x03
    FN = 0x04
    DNS = 0x05


class Param(IntEnum):
    P1_DirectlyFetchAddress = 0x00  # Return address directly from the wallet
    P1_ShowFetchAddress = 0x01  # Return address from the wallet after showing it
    P1_InitTransactionData = 0x00  # First transaction data block for signing
    P1_ContTransactionData = 0x80  # Subsequent transaction data block for signing
    P2_DiscardAddressChainCode = (
        0x00  # Do not return the chain code along with the address
    )
    P1_UNUSED = 0x00  # Default value
    P2_UNUSED = 0x00  # Default value


class PKIPubKeyUsage(IntEnum):
    PUBKEY_USAGE_GENUINE_CHECK = 0x01
    PUBKEY_USAGE_EXCHANGE_PAYLOAD = 0x02
    PUBKEY_USAGE_NFT_METADATA = 0x03
    PUBKEY_USAGE_TRUSTED_NAME = 0x04
    PUBKEY_USAGE_BACKUP_PROVIDER = 0x05
    PUBKEY_USAGE_RECOVER_ORCHESTRATOR = 0x06
    PUBKEY_USAGE_PLUGIN_METADATA = 0x07
    PUBKEY_USAGE_COIN_META = 0x08
    PUBKEY_USAGE_SEED_ID_AUTH = 0x09
    PUBKEY_USAGE_TX_SIMU_SIGNER = 0x0A
    PUBKEY_USAGE_CALLDATA = 0x0B
    PUBKEY_USAGE_NETWORK = 0x0C


class PKIClient:
    _CLA: int = 0xB0
    _INS: int = 0x06

    def __init__(self, backend: BackendInterface) -> None:
        self._backend = backend

    def send_certificate(self, p1: PKIPubKeyUsage, payload: bytes) -> None:
        try:
            response = self.send_raw(p1, payload)
            assert response.status == StatusCode.STATUS_OK
        except ExceptionRAPDU as err:
            if err.status == StatusCode.STATUS_NOT_IMPLEMENTED:
                print("Ledger-PKI APDU not yet implemented. Legacy path will be used")

    def send_raw(self, p1: PKIPubKeyUsage, payload: bytes) -> RAPDU:
        header = bytearray()
        header.append(self._CLA)
        header.append(self._INS)
        header.append(p1)
        header.append(0x00)
        header.append(len(payload))
        return self._backend.exchange_raw(header + payload)


class CeloClient:
    backend: BackendInterface

    def __init__(self, backend):
        self._backend = backend
        self.device = backend.device
        self._cmd_builder = CommandBuilder()
        self.pki_client = PKIClient(self._backend)

    # These are used to send raw APDUs to the device
    def _exchange_async(self, payload: bytes):
        return self._backend.exchange_async_raw(payload)

    def _exchange(self, payload: bytes):
        return self._backend.exchange_raw(payload)

    def response(self) -> Optional[RAPDU]:
        return self._backend.last_async_response

    def get_version(self) -> bytes:
        version: RAPDU = self._backend.exchange(
            CLA, INS.INS_GET_APP_CONFIGURATION, Param.P1_UNUSED, Param.P2_UNUSED
        )
        return version

    def get_public_addr(
        self, derivation_path: str, show: bool, chaincode: bool
    ) -> bytes:
        p1 = Param.P1_ShowFetchAddress if show else Param.P1_DirectlyFetchAddress
        p2 = Param.P2_UNUSED if chaincode else Param.P2_DiscardAddressChainCode
        derivation_path_bytes = pack_derivation_path(derivation_path)
        return self._backend.exchange_async(
            CLA, INS.INS_GET_PUBLIC_KEY, p1, p2, derivation_path_bytes
        )

    @contextmanager
    def send_in_chunk_async(
        self, instruction: int, payload: bytes
    ) -> Generator[None, None, None]:
        p1 = Param.P1_InitTransactionData
        while len(payload) > 0:
            chunk = MAX_CHUNK_SIZE
            if chunk > len(payload):
                chunk = len(payload)

            with self._backend.exchange_async(
                CLA, instruction, p1, Param.P2_UNUSED, payload[:chunk]
            ):
                yield

            payload = payload[chunk:]
            p1 = Param.P1_ContTransactionData

    @contextmanager
    def derive_address_async(
        self, derivation_path: bytes, show: bool, chaincode: bool
    ) -> Generator[None, None, None]:
        p1 = Param.P1_ShowFetchAddress if show else Param.P1_DirectlyFetchAddress
        p2 = Param.P2_UNUSED if chaincode else Param.P2_DiscardAddressChainCode

        with self._backend.exchange_async(
            CLA, INS.INS_GET_PUBLIC_KEY, p1, p2, derivation_path
        ):
            yield

    @contextmanager
    def sign_data_async(
        self, derivation_path: bytes, data: str
    ) -> Generator[None, None, None]:
        payload: bytes = derivation_path
        payload += len(data).to_bytes(4, byteorder="big")
        payload += str.encode(data)

        with self.send_in_chunk_async(INS.INS_SIGN_PERSONAL_MESSAGE, payload):
            yield

    @contextmanager
    def sign_transaction_async(
        self, derivation_path: bytes, arg_list: List[str]
    ) -> Generator[None, None, None]:
        encoded = rlp.encode(arg_list)

        payload: bytes = derivation_path
        payload += encoded

        with self.send_in_chunk_async(INS.INS_SIGN, payload):
            yield

    @contextmanager
    def sign_transaction_with_rawTx_async(
        self, derivation_path: bytes, rawTx: str
    ) -> Generator[None, None, None]:
        encoded = bytes.fromhex(rawTx)
        payload: bytes = derivation_path
        payload += encoded

        with self.send_in_chunk_async(INS.INS_SIGN, payload):
            yield

    # eip712
    def get_challenge(self):
        return self._exchange(self._cmd_builder.get_challenge())

    def eip712_send_struct_def_struct_name(self, name: str):
        return self._exchange_async(
            self._cmd_builder.eip712_send_struct_def_struct_name(name)
        )

    def eip712_send_struct_def_struct_field(
        self,
        field_type: EIP712FieldType,
        type_name: str,
        type_size: int,
        array_levels: list,
        key_name: str,
    ):
        return self._exchange_async(
            self._cmd_builder.eip712_send_struct_def_struct_field(
                field_type, type_name, type_size, array_levels, key_name
            )
        )

    def eip712_send_struct_impl_root_struct(self, name: str):
        return self._exchange_async(
            self._cmd_builder.eip712_send_struct_impl_root_struct(name)
        )

    def eip712_send_struct_impl_array(self, size: int):
        return self._exchange_async(
            self._cmd_builder.eip712_send_struct_impl_array(size)
        )

    def eip712_send_struct_impl_struct_field(self, raw_value: bytes):
        chunks = self._cmd_builder.eip712_send_struct_impl_struct_field(
            bytearray(raw_value)
        )
        for chunk in chunks[:-1]:
            self._exchange(chunk)
        return self._exchange_async(chunks[-1])

    def eip712_sign_new(self, bip32_path: str):
        return self._exchange_async(self._cmd_builder.eip712_sign_new(bip32_path))

    def eip712_sign_legacy(
        self, bip32_path: str, domain_hash: bytes, message_hash: bytes
    ):
        return self._exchange_async(
            self._cmd_builder.eip712_sign_legacy(bip32_path, domain_hash, message_hash)
        )

    def eip712_filtering_activate(self):
        return self._exchange_async(self._cmd_builder.eip712_filtering_activate())

    def eip712_filtering_discarded_path(self, path: str):
        return self._exchange(self._cmd_builder.eip712_filtering_discarded_path(path))

    def eip712_filtering_message_info(self, name: str, filters_count: int, sig: bytes):
        return self._exchange_async(
            self._cmd_builder.eip712_filtering_message_info(name, filters_count, sig)
        )

    def eip712_filtering_amount_join_token(
        self, token_idx: int, sig: bytes, discarded: bool
    ):
        return self._exchange_async(
            self._cmd_builder.eip712_filtering_amount_join_token(
                token_idx, sig, discarded
            )
        )

    def eip712_filtering_amount_join_value(
        self, token_idx: int, name: str, sig: bytes, discarded: bool
    ):
        return self._exchange_async(
            self._cmd_builder.eip712_filtering_amount_join_value(
                token_idx, name, sig, discarded
            )
        )

    def eip712_filtering_datetime(self, name: str, sig: bytes, discarded: bool):
        return self._exchange_async(
            self._cmd_builder.eip712_filtering_datetime(name, sig, discarded)
        )

    def eip712_filtering_trusted_name(
        self,
        name: str,
        name_type: list[int],
        name_source: list[int],
        sig: bytes,
        discarded: bool,
    ):
        return self._exchange_async(
            self._cmd_builder.eip712_filtering_trusted_name(
                name, name_type, name_source, sig, discarded
            )
        )

    def eip712_filtering_raw(self, name: str, sig: bytes, discarded: bool):
        return self._exchange_async(
            self._cmd_builder.eip712_filtering_raw(name, sig, discarded)
        )

    @staticmethod
    def get_token_signature(ticker: str, chain_id: int) -> Optional[bytes]:
        """
        Get the token signature for a given ticker and chain ID.

        Args:
            ticker: Token ticker symbol
            chain_id: Blockchain chain ID

        Returns:
            Signature bytes if found, None otherwise
        """
        signature_hex = TOKEN_SIGNATURES.get((ticker, chain_id))
        if signature_hex:
            return bytes.fromhex(signature_hex)
        return None

    def provide_token_metadata(
        self,
        ticker: str,
        addr: bytes,
        decimals: int,
        chain_id: int,
        sig: Optional[bytes] = None,
    ) -> RAPDU:

        # pylint: disable=line-too-long
        if self.device.type == DeviceType.NANOSP:
            cert_apdu = "01010102010211040000000212010013020002140101160400000000200B45524332305F546F6B656E300200063101083201213321024CCA8FAD496AA5040A00A7EB2F5CC3B85376D88BA147A7D7054A99C64056188734010135010310040102000015473045022100C15795C2AE41E6FAE6B1362EE1AE216428507D7C1D6939B928559CC7A1F6425C02206139CF2E133DD62F3E00F183E42109C9853AC62B6B70C5079B9A80DBB9D54AB5"  # noqa: E501
        elif self.device.type == DeviceType.NANOX:
            cert_apdu = "01010102010211040000000212010013020002140101160400000000200B45524332305F546F6B656E300200063101083201213321024CCA8FAD496AA5040A00A7EB2F5CC3B85376D88BA147A7D7054A99C64056188734010135010215473045022100E3B956F93FBFF0D41908483888F0F75D4714662A692F7A38DC6C41A13294F9370220471991BECB3CA4F43413CADC8FF738A8CC03568BFA832B4DCFE8C469080984E5"  # noqa: E501
        elif self.device.type == DeviceType.STAX:
            cert_apdu = "01010102010211040000000212010013020002140101160400000000200B45524332305F546F6B656E300200063101083201213321024CCA8FAD496AA5040A00A7EB2F5CC3B85376D88BA147A7D7054A99C6405618873401013501041546304402206731FCD3E2432C5CA162381392FD17AD3A41EEF852E1D706F21A656AB165263602204B89FAE8DBAF191E2D79FB00EBA80D613CB7EDF0BE960CB6F6B29D96E1437F5F"  # noqa: E501
        elif self.device.type == DeviceType.FLEX:
            cert_apdu = "01010102010211040000000212010013020002140101160400000000200B45524332305F546F6B656E300200063101083201213321024CCA8FAD496AA5040A00A7EB2F5CC3B85376D88BA147A7D7054A99C64056188734010135010515473045022100B59EA8B958AA40578A6FBE9BBFB761020ACD5DBD8AA863C11DA17F42B2AFDE790220186316059EFA58811337D47C7F815F772EA42BBBCEA4AE123D1118C80588F5CB"  # noqa: E501
        else:
            print(f"Invalid device '{self.device.name}'")
            cert_apdu = ""
        # pylint: enable=line-too-long
        if cert_apdu:
            self.pki_client.send_certificate(
                PKIPubKeyUsage.PUBKEY_USAGE_COIN_META, bytes.fromhex(cert_apdu)
            )

        if sig is None:
            sig = self.get_token_signature(ticker, chain_id)
            if sig is None:
                sig = bytes()

        return self._exchange(
            self._cmd_builder.provide_erc20_token_information(
                ticker, addr, decimals, chain_id, sig
            )
        )
