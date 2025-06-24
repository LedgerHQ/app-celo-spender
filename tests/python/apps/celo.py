from typing import List, Generator
from enum import IntEnum
from contextlib import contextmanager

from ragger.backend.interface import BackendInterface, RAPDU

from .celo_cmd_builder import *
import rlp


class INS(IntEnum):
    INS_GET_PUBLIC_KEY = 0x02
    INS_SIGN = 0x04
    INS_GET_APP_CONFIGURATION = 0x06
    INS_SIGN_PERSONAL_MESSAGE = 0x08
    INS_PROVIDE_ERC20_TOKEN_INFORMATION = 0x0A
    INS_GET_APP_TYPE = 0x0C


CLA = 0xE0

MAX_CHUNK_SIZE = 255


class StatusCode(IntEnum):
    STATUS_OK = 0x9000
    STATUS_DEPRECATED = 0x6501
    STATUS_ERROR_IN_DATA = 0x6A80


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


class CeloClient:
    backend: BackendInterface

    def __init__(self, backend):
        self._client = backend

    def get_version(self) -> bytes:
        version: RAPDU = self._client.exchange(
            CLA, INS.INS_GET_APP_CONFIGURATION, Param.P1_UNUSED, Param.P2_UNUSED
        )
        return version

    def get_public_addr(
        self, derivation_path: bytes, show: bool, chaincode: bool
    ) -> bytes:
        p1 = Param.P1_ShowFetchAddress if show else Param.P1_DirectlyFetchAddress
        p2 = Param.P2_UNUSED if chaincode else Param.P2_DiscardAddressChainCode

        return self._client.exchange_async(
            CLA, INS.INS_GET_PUBLIC_KEY, p1, p2, derivation_path
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

            with self._client.exchange_async(
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

        with self._client.exchange_async(
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
