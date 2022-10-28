from time import sleep
import base58
from pathlib import Path

from ragger.backend.interface import RAPDU, RaisePolicy

from .apps.celo import CeloClient, StatusCode
from .apps.celo_cmd_builder import *
from .apps.celo_utils import CELO_PACKED_DERIVATION_PATH
from .utils import validate_displayed_message, approve_message, get_async_response
from ragger.navigator import NavInsID, NavIns

import pytest

TESTS_ROOT_DIR = Path(__file__).parent

TOKEN_TRANSFER_ID = 0xa9059cbb
TOKEN_TRANSFER_WITH_COMMENT_ID = 0xe1d6aceb
LOCK_METHOD_ID = 0xf83d08ba
VOTE_METHOD_ID = 0x580d747a
ACTIVATE_METHOD_ID = 0x1c5a9d9c
REVOKE_PENDING_METHOD_ID = 0x9dfb6081
REVOKE_ACTIVE_METHOD_ID = 0x6e198475
UNLOCK_METHOD_ID = 0x6198e339
WITHDRAW_METHOD_ID = 0x2e1a7d4d
RELOCK_METHOD_ID = 0xb2fb30cb
CREATE_ACCOUNT_METHOD_ID = 0x9dca362f

payloads = [
        "",
        TOKEN_TRANSFER_ID.to_bytes(4, byteorder='big') + b'00' * 32,
        UNLOCK_METHOD_ID.to_bytes(4, byteorder='big'),
        LOCK_METHOD_ID.to_bytes(4, byteorder='big'),
        VOTE_METHOD_ID.to_bytes(4, byteorder='big') + b'00' * 64,
        ACTIVATE_METHOD_ID.to_bytes(4, byteorder='big') + b'00' * 16,
        REVOKE_PENDING_METHOD_ID.to_bytes(4, byteorder='big') + b'00' * 80,
        UNLOCK_METHOD_ID.to_bytes(4, byteorder='big') + b'00' * 16,
        WITHDRAW_METHOD_ID.to_bytes(4, byteorder='big') + b'00' * 16,
        RELOCK_METHOD_ID.to_bytes(4, byteorder='big') + b'00' * 32,
        CREATE_ACCOUNT_METHOD_ID.to_bytes(4, byteorder='big')
        ]

def get_nano_review_instructions(num_screen_skip):
    instructions = [NavIns(NavInsID.RIGHT_CLICK)] * num_screen_skip
    instructions.append(NavIns(NavInsID.BOTH_CLICK))
    return instructions


def get_fat_review_instructions(num_screen_skip):
    instructions = [NavIns(NavInsID.USE_CASE_REVIEW_TAP)] * num_screen_skip
    instructions.append(NavIns(NavInsID.USE_CASE_REVIEW_CONFIRM))
    instructions.append(NavIns(NavInsID.USE_CASE_STATUS_WAIT))
    return instructions

@pytest.mark.parametrize("show", [False, True])
@pytest.mark.parametrize("chaincode", [False, True])
def test_celo_derive_address(test_name, client, firmware, show, chaincode, navigator): 
    celo = CeloClient(client)

    if firmware.device == "nanos":
        instructions = get_nano_review_instructions(5)
    elif firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(3)
    else:
        instructions = [
            #NavIns(NavInsID.USE_CASE_REVIEW_TAP),
            NavIns(NavInsID.USE_CASE_ADDRESS_CONFIRMATION_SHOW_QR),
            NavIns(NavInsID.USE_CASE_ADDRESS_CONFIRMATION_EXIT_QR),
            NavIns(NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CONFIRM),
            NavIns(NavInsID.USE_CASE_STATUS_WAIT)
        ]

    with celo.derive_address_async(CELO_PACKED_DERIVATION_PATH, show, chaincode):
        if show:
            navigator.navigate_and_compare(TESTS_ROOT_DIR,
                                           test_name,
                                           instructions)

    response : bytes = get_async_response(client)

    assert (response.status == StatusCode.STATUS_OK)
    sleep(0.1)


def test_celo_get_version(client, firmware):
    celo = CeloClient(client)
    response = celo.get_version()

    assert (response.status == StatusCode.STATUS_OK)
    sleep(0.1)


def test_sign_data(test_name, client, firmware, navigator):
    celo = CeloClient(client)
    if firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(3)
    else:
        instructions = get_fat_review_instructions(1)

    with celo.sign_data_async(CELO_PACKED_DERIVATION_PATH, "1234567890"):
        navigator.navigate_and_compare(TESTS_ROOT_DIR, test_name, instructions)

    response : bytes = get_async_response(client)

    assert (response.status == StatusCode.STATUS_OK)
    assert (len(response.data) == 65)
    sleep(0.1)


@pytest.mark.parametrize("payload", payloads)
def test_sign_transaction(test_name, client, firmware, payload, navigator):
    celo = CeloClient(client)
    if firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(3)
    else:
        instructions = [NavIns(NavInsID.USE_CASE_CHOICE_CONFIRM)]
        instructions += get_fat_review_instructions(5)
    with celo.sign_transaction_async(CELO_PACKED_DERIVATION_PATH,
            [
                "1234",                  #Nonce
                "1234",                  #GasPrice
                "1234",                  #StartGas
                "",                      #FeeCurrency
                "12345678901234567890",  #GatewayTo
                "1234",                  #GatewayFee 
                "12345678901234567890",  #To
                "",                      #Value
                payload,
                "",                      #V
                "",                      #R
                "",                      #S
                ""                       #Done
                ]
            ):
        navigator.navigate_and_compare(TESTS_ROOT_DIR, test_name, instructions)
    response : bytes = get_async_response(client)

    assert (response.status == StatusCode.STATUS_OK)
    sleep(0.1)

@pytest.mark.parametrize("payload", ["", "1234"])
def test_sign_transaction_no_gtw(client, firmware, payload):
    celo = CeloClient(client)
    with celo.sign_transaction_async(CELO_PACKED_DERIVATION_PATH,
            [
                "1234",                 #Nonce
                "1234",                 #GasPrice
                "1234",                 #StartGas
                "",                     #FeeCurrency
                "",                     #GatewayTo
                "1234",                 #GatewayFee 
                "12345678901234567890", #To
                "",                     #Value
                payload,
                "",                     #V
                "",                     #R
                "",                     #S
                ""                      #Done
                ]
            ):
        validate_displayed_message(client, 3 + 1)
        approve_message(client)

    response : bytes = get_async_response(client)

    assert (response.status == StatusCode.STATUS_OK)
    sleep(0.1)

