from pathlib import Path

from .apps.celo import CeloClient, StatusCode
from .apps.celo_utils import CELO_PACKED_DERIVATION_PATH
from .utils import get_async_response, get_nano_review_instructions
from ragger.navigator import NavInsID, NavIns
from ledgered.devices import DeviceType

import pytest

TESTS_ROOT_DIR = Path(__file__).parent


@pytest.mark.parametrize("show", [False, True])
@pytest.mark.parametrize("chaincode", [False, True])
def test_celo_derive_address(test_name, backend, show, chaincode, navigator):
    celo = CeloClient(backend)

    if backend.device.is_nano:
        instructions = get_nano_review_instructions(2)
    else:

        if backend.device.type == DeviceType.STAX:
            qr_code_position = (64, 521)
        elif backend.device.type == DeviceType.FLEX:
            qr_code_position = (76, 463)
        elif backend.device.type == DeviceType.APEX_P:
            qr_code_position = (45, 289)

        instructions = [
            NavIns(NavInsID.SWIPE_CENTER_TO_LEFT),
            NavIns(NavInsID.TOUCH, qr_code_position),
            NavIns(NavInsID.USE_CASE_ADDRESS_CONFIRMATION_EXIT_QR),
            NavIns(NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CONFIRM),
            NavIns(NavInsID.USE_CASE_STATUS_DISMISS)
        ]

    with celo.derive_address_async(CELO_PACKED_DERIVATION_PATH, show, chaincode):
        if show:
            navigator.navigate_and_compare(TESTS_ROOT_DIR,
                                           test_name,
                                           instructions)

    response: bytes = get_async_response(backend)

    assert (response.status == StatusCode.STATUS_OK)


def test_celo_get_version(backend):
    celo = CeloClient(backend)
    response = celo.get_version()

    assert (response.status == StatusCode.STATUS_OK)


def navigate_message_review_and_approve(test_name, backend, navigator):
    """Approves a message review without counting screens.

    The review shows the whole 32-byte digest, so how many pages it spans is a device
    detail these tests must not encode. On Nano the approval step cannot be found by
    name either — it reads "Sign message", exactly like the flow's first step — so this
    walks to the Cancel step and steps back one screen to land on approval.
    """
    if backend.device.is_nano:
        navigator.navigate_until_text_and_compare(NavInsID.RIGHT_CLICK,
                                                  [NavInsID.LEFT_CLICK,
                                                   NavInsID.BOTH_CLICK],
                                                  "Cancel",
                                                  TESTS_ROOT_DIR,
                                                  test_name)
    else:
        navigator.navigate_until_text_and_compare(NavInsID.SWIPE_CENTER_TO_LEFT,
                                                  [NavInsID.USE_CASE_REVIEW_CONFIRM,
                                                   NavInsID.USE_CASE_STATUS_DISMISS],
                                                  "Sign message?",
                                                  TESTS_ROOT_DIR,
                                                  test_name)


def test_sign_data(test_name, backend, navigator):
    celo = CeloClient(backend)

    with celo.sign_data_async(CELO_PACKED_DERIVATION_PATH, "1234567890"):
        navigate_message_review_and_approve(test_name, backend, navigator)

    response: bytes = get_async_response(backend)

    assert (response.status == StatusCode.STATUS_OK)
    assert (len(response.data) == 65)


def test_sign_data_multibyte_utf8(test_name, backend, navigator):
    # The APDU length field counts bytes, not characters. This message is 17 characters
    # and 21 bytes, so a client that declares the character count understates it and the
    # app refuses the chunk — the case that kept non-ASCII messages untested.
    message = "Paiement café 10€"
    assert len(message.encode("utf-8")) != len(message)

    celo = CeloClient(backend)
    with celo.sign_data_async(CELO_PACKED_DERIVATION_PATH, message):
        navigate_message_review_and_approve(test_name, backend, navigator)

    response: bytes = get_async_response(backend)

    assert (response.status == StatusCode.STATUS_OK)
    assert (len(response.data) == 65)

def test_show_settings_menu(test_name, backend, navigator):
    instructions = []
    if backend.device.is_nano:
        instructions = [
            NavInsID(NavInsID.RIGHT_CLICK),
            NavInsID(NavInsID.RIGHT_CLICK),
            NavInsID(NavInsID.BOTH_CLICK),
            NavInsID(NavInsID.RIGHT_CLICK),
            NavInsID(NavInsID.RIGHT_CLICK),
            NavInsID(NavInsID.BOTH_CLICK),
        ]
    else:
        instructions = [
            NavIns(NavInsID.USE_CASE_HOME_SETTINGS),
            NavIns(NavInsID.USE_CASE_SETTINGS_NEXT),
            NavIns(NavInsID.USE_CASE_SETTINGS_MULTI_PAGE_EXIT),
        ]

    navigator.navigate_and_compare(
        TESTS_ROOT_DIR,
        test_name,
        instructions,
        screen_change_before_first_instruction = False
    )
