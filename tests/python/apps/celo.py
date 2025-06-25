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

    def provide_tx_simulation(self, simu_params: TxSimu) -> RAPDU:
        # pylint: disable=line-too-long
        if self.device.type == DeviceType.NANOSP:
            cert_apdu = "01010102010211040000000212010013020002140101160400000000200A573343204C65646765723002000531010A320121332103DA154C97512390BDFDD0E2178523EF7B2486B7A7DD5B507079D2F3641AEF550134010135010315473045022100EABC9E26361DD551E30F52604884E5D0DAEF9EDD63848C45DA0B446DEE870BF002206AC308F46D04E5B23CC8D62F9C062E3AF931E3EEF9C2509AFA768A891CA8F10B"  # noqa: E501
        elif self.device.type == DeviceType.NANOX:
            cert_apdu = "01010102010211040000000212010013020002140101160400000000200A573343204C65646765723002000531010A320121332103DA154C97512390BDFDD0E2178523EF7B2486B7A7DD5B507079D2F3641AEF550134010135010215473045022100B9EC810718F85C110CF60E7C5A8A6A4F783F3E0918E93861A8FCDCE7CFF4D405022047BD29E3F2B8EFD3FC7451FA19EE3147C38BEF83246DC396E7A10B2D2A44DB30"  # noqa: E501
        elif self.device.type == DeviceType.STAX:
            cert_apdu = "01010102010211040000000212010013020002140101160400000000200A573343204C65646765723002000531010A320121332103DA154C97512390BDFDD0E2178523EF7B2486B7A7DD5B507079D2F3641AEF550134010135010415473045022100C490C9DD99F7D1A39D3AE9D448762DEAA17694C9BCF00454503498D2BA883DFE02204AEAD903C2B3A106206D7C2B1ACAA6DD20B6B41EE6AEF38F060F05EC4D7813BB"  # noqa: E501
        elif self.device.type == DeviceType.FLEX:
            cert_apdu = "01010102010211040000000212010013020002140101160400000000200A573343204C65646765723002000531010A320121332103DA154C97512390BDFDD0E2178523EF7B2486B7A7DD5B507079D2F3641AEF550134010135010515473045022100FFDE724191BAA18250C7A93404D57CE13465797979EAF64239DB221BA679C5A402206E02953F47D32299F82616713D3DBA39CF8A09EF948B23446A6DDE610F80D066"  # noqa: E501
        else:
            print(f"Invalid device '{self.device.name}'")
            cert_apdu = ""
        # pylint: enable=line-too-long
        if cert_apdu:
            self.pki_client.send_certificate(
                PKIPubKeyUsage.PUBKEY_USAGE_TX_SIMU_SIGNER, bytes.fromhex(cert_apdu)
            )

        if not simu_params.from_addr:
            with self.get_public_addr(False):
                pass
            response = self.response()
            assert response
            _, from_addr, _ = pk_addr(response.data)
            simu_params.from_addr = from_addr

        chunks = self._cmd_builder.provide_tx_simulation(simu_params.serialize())
        for chunk in chunks[:-1]:
            self._exchange(chunk)
        return self._exchange(chunks[-1])

    def provide_proxy_info(self, payload: bytes) -> RAPDU:
        # pylint: disable=line-too-long
        if self.device.type == DeviceType.NANOSP:
            cert_apdu = "01010102010211040000000212010013020002140101160400000000200863616C6C646174613002000831010B32012133210381C0821E2A14AC2546FB0B9852F37CA2789D7D76483D79217FB36F51DCE1E7B434010135010315463044022076DD2EAB72E69D440D6ED8290C8C37E39F54294C23FF0F8520F836E7BE07455C02201D9A8A75223C1ADA1D9D00966A12EBB919D0BBF2E66F144C83FADCAA23672566"  # noqa: E501
        elif self.device.type == DeviceType.NANOX:
            cert_apdu = "01010102010211040000000212010013020002140101160400000000200863616C6C646174613002000831010B32012133210381C0821E2A14AC2546FB0B9852F37CA2789D7D76483D79217FB36F51DCE1E7B434010135010215463044022077FF9625006CB8A4AD41A4B04FF2112E92A732BD263CCE9B97D8E7D2536D04300220445B8EE3616FB907AA5E68359275E94D0A099C3E32A4FC8B3669C34083671F2F"  # noqa: E501
        elif self.device.type == DeviceType.STAX:
            cert_apdu = "01010102010211040000000212010013020002140101160400000000200863616C6C646174613002000831010B32012133210381C0821E2A14AC2546FB0B9852F37CA2789D7D76483D79217FB36F51DCE1E7B434010135010415473045022100A88646AD72CA012D5FDAF8F6AE0B7EBEF079212768D57323CB5B57CADD9EB20D022005872F8EA06092C9783F01AF02C5510588FB60CBF4BA51FB382B39C1E060BB6B"  # noqa: E501
        elif self.device.type == DeviceType.FLEX:
            cert_apdu = "01010102010211040000000212010013020002140101160400000000200863616C6C646174613002000831010B32012133210381C0821E2A14AC2546FB0B9852F37CA2789D7D76483D79217FB36F51DCE1E7B43401013501051546304402205305BDDDAD0284A2EAC2A9BE4CEF6604AE9415C5F46883448F5F6325026234A3022001ED743BCF33CCEB070FDD73C3D3FCC2CEE5AB30A5C3EB7D2A8D21C6F58D493F"  # noqa: E501
        # pylint: enable=line-too-long
        self.pki_client.send_certificate(
            PKIPubKeyUsage.PUBKEY_USAGE_CALLDATA, bytes.fromhex(cert_apdu)
        )

        chunks = self._cmd_builder.provide_proxy_info(payload)
        for chunk in chunks[:-1]:
            self._exchange(chunk)
        return self._exchange(chunks[-1])

    def _provide_trusted_name_common(
        self, payload: bytes, name_source: TrustedNameSource
    ) -> RAPDU:
        payload += format_tlv(FieldTag.STRUCT_TYPE, 3)  # TrustedName
        cert_apdu = ""
        if name_source == TrustedNameSource.CAL:
            # pylint: disable=line-too-long
            if self.device.type == DeviceType.NANOSP:
                cert_apdu = "010101020102110400000002120100130200021401011604000000002010547275737465645F4E616D655F43414C300200093101043201213321024CCA8FAD496AA5040A00A7EB2F5CC3B85376D88BA147A7D7054A99C6405618873401013501031546304402202397E6D4F5A9C13532810619EB766CB41919F7A814935CA207429966E41CE80202202E43159A04BB0596A6B1F0DDE1A12931EA56156751586BEE8FDCAB54EEE36883"  # noqa: E501
            elif self.device.type == DeviceType.NANOX:
                cert_apdu = "010101020102110400000002120100130200021401011604000000002010547275737465645F4E616D655F43414C300200093101043201213321024CCA8FAD496AA5040A00A7EB2F5CC3B85376D88BA147A7D7054A99C64056188734010135010215473045022100BCAED6896A3413657F7F936F53FFF5BD7E30498B1109EAB878135F6DD4AAE555022056913EF42F166BEFEEDDB06C1F7AEEDE8712828F37B916E3E6DA7AE0809558E4"  # noqa: E501
            elif self.device.type == DeviceType.STAX:
                cert_apdu = "010101020102110400000002120100130200021401011604000000002010547275737465645F4E616D655F43414C300200093101043201213321024CCA8FAD496AA5040A00A7EB2F5CC3B85376D88BA147A7D7054A99C64056188734010135010415473045022100ABA9D58446EE81EB073DA91941989DD7E133556D58DE2BCBA59E46253DB448B102201DF8AE930A9E318B50576D8922503A5D3EC84C00C332A7C8FF7CD48708751840"  # noqa: E501
            elif self.device.type == DeviceType.FLEX:
                cert_apdu = "010101020102110400000002120100130200021401011604000000002010547275737465645F4E616D655F43414C300200093101043201213321024CCA8FAD496AA5040A00A7EB2F5CC3B85376D88BA147A7D7054A99C6405618873401013501051546304402206DC9F82C53F3B13400D3E343E3C8C81868E8C73B1EF2655D07891064B7AC3166022069A36E4059D75C93E488A5D58C02BCA9C80C081F77B31C5EDCF07F1A500C565A"  # noqa: E501
            else:
                print(f"Invalid device '{self.device.name}'")
                # pylint: enable=line-too-long
            key_id = 9
            key = Key.CAL
        else:
            # pylint: disable=line-too-long
            if self.device.type == DeviceType.NANOSP:
                cert_apdu = "01010102010211040000000212010013020002140101160400000000200C547275737465645F4E616D6530020007310104320121332102B91FBEC173E3BA4A714E014EBC827B6F899A9FA7F4AC769CDE284317A00F4F6534010135010315473045022100F394484C045418507E0F76A3231F233B920C733D3E5BB68AFBAA80A55195F70D022012BC1FD796CD2081D8355DEEFA051FBB9329E34826FF3125098F4C6A0C29992A"  # noqa: E501
            elif self.device.type == DeviceType.NANOX:
                cert_apdu = "01010102010211040000000212010013020002140101160400000000200C547275737465645F4E616D6530020007310104320121332102B91FBEC173E3BA4A714E014EBC827B6F899A9FA7F4AC769CDE284317A00F4F65340101350102154730450221009D97646C49EE771BE56C321AB59C732E10D5D363EBB9944BF284A3A04EC5A14102200633518E851984A7EA00C5F81EDA9DAA58B4A6C98E57DA1FBB9074AEFF0FE49F"  # noqa: E501
            elif self.device.type == DeviceType.STAX:
                cert_apdu = "01010102010211040000000212010013020002140101160400000000200C547275737465645F4E616D6530020007310104320121332102B91FBEC173E3BA4A714E014EBC827B6F899A9FA7F4AC769CDE284317A00F4F6534010135010415473045022100A57DC7AB3F0E38A8D10783C7449024D929C60843BB75E5FF7B8088CB71CB130C022045A03E6F501F3702871466473BA08CE1F111357ED9EF395959733477165924C4"  # noqa: E501
            elif self.device.type == DeviceType.FLEX:
                cert_apdu = "01010102010211040000000212010013020002140101160400000000200C547275737465645F4E616D6530020007310104320121332102B91FBEC173E3BA4A714E014EBC827B6F899A9FA7F4AC769CDE284317A00F4F6534010135010515473045022100D5BB77756C3D7C1B4254EA8D5351B94A89B13BA69C3631A523F293A10B7144B302201519B29A882BB22DCDDF6BE79A9CBA76566717FA877B7CA4B9CC40361A2D579E"  # noqa: E501
            else:
                print(f"Invalid device '{self.device.name}'")
                # pylint: enable=line-too-long
            key_id = 7
            key = Key.TRUSTED_NAME

        if cert_apdu:
            self.pki_client.send_certificate(
                PKIPubKeyUsage.PUBKEY_USAGE_TRUSTED_NAME, bytes.fromhex(cert_apdu)
            )

        payload += format_tlv(FieldTag.SIGNER_KEY_ID, key_id)  # test key
        payload += format_tlv(FieldTag.SIGNER_ALGO, 1)  # secp256k1
        payload += format_tlv(FieldTag.DER_SIGNATURE, sign_data(key, payload))
        chunks = self._cmd_builder.provide_trusted_name(payload)
        for chunk in chunks[:-1]:
            self._exchange(chunk)
        return self._exchange(chunks[-1])

    def provide_trusted_name_v2(
        self,
        addr: bytes,
        name: str,
        name_type: TrustedNameType,
        name_source: TrustedNameSource,
        chain_id: int,
        nft_id: Optional[int] = None,
        challenge: Optional[int] = None,
        not_valid_after: Optional[tuple[int, int, int]] = None,
    ) -> RAPDU:
        payload = format_tlv(FieldTag.STRUCT_VERSION, 2)
        payload += format_tlv(FieldTag.TRUSTED_NAME, name)
        payload += format_tlv(FieldTag.ADDRESS, addr)
        payload += format_tlv(FieldTag.TRUSTED_NAME_TYPE, name_type)
        payload += format_tlv(FieldTag.TRUSTED_NAME_SOURCE, name_source)
        payload += format_tlv(FieldTag.CHAIN_ID, chain_id)
        if nft_id is not None:
            payload += format_tlv(FieldTag.TRUSTED_NAME_NFT_ID, nft_id)
        if challenge is not None:
            payload += format_tlv(FieldTag.CHALLENGE, challenge)
        if not_valid_after is not None:
            assert len(not_valid_after) == 3
            payload += format_tlv(
                FieldTag.NOT_VALID_AFTER, struct.pack("BBB", *not_valid_after)
            )
        return self._provide_trusted_name_common(payload, name_source)
