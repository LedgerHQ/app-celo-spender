from pathlib import Path

from .apps.celo import CeloClient, StatusCode
from .apps.celo_utils import CELO_PACKED_DERIVATION_PATH
from .utils import get_async_response, get_nano_review_instructions, get_stax_review_instructions, get_stax_review_instructions_with_warning
from ragger.navigator import NavInsID, NavIns

import pytest

TESTS_ROOT_DIR = Path(__file__).parent


@pytest.mark.parametrize("show", [False, True])
@pytest.mark.parametrize("chaincode", [False, True])
def test_celo_derive_address(test_name, backend, firmware, show, chaincode, navigator): 
    celo = CeloClient(backend)

    if firmware.device == "nanos":
        instructions = get_nano_review_instructions(4)
    elif firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(2)
    else:
        instructions = [
            NavIns(NavInsID.SWIPE_CENTER_TO_LEFT),
            NavIns(NavInsID.TOUCH, (64, 521)),
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


def test_celo_get_version(backend, firmware):
    celo = CeloClient(backend)
    response = celo.get_version()

    assert (response.status == StatusCode.STATUS_OK)


def test_sign_data(test_name, backend, firmware, navigator):
    celo = CeloClient(backend)
    if firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(2)
    else:
        instructions = get_stax_review_instructions(1)

    with celo.sign_data_async(CELO_PACKED_DERIVATION_PATH, "1234567890"):
        navigator.navigate_and_compare(TESTS_ROOT_DIR, test_name, instructions)

    response: bytes = get_async_response(backend)

    assert (response.status == StatusCode.STATUS_OK)
    assert (len(response.data) == 65)
