from pathlib import Path
from .apps.celo import CeloClient, StatusCode
from .apps.celo_utils import ETH_PACKED_DERIVATION_PATH
from .utils import get_async_response, get_nano_review_instructions, get_stax_review_instructions, get_stax_review_instructions_with_warning
from ragger.navigator import NavInsID, NavIns

import pytest

TESTS_ROOT_DIR = Path(__file__).parent

# Transaction types
TX_TYPE_EIP1559 = 2
TX_TYPE_CIP64 = 123


def sign_transaction_with_rawTx(test_name, backend, navigator, instructions, rawTx):
    celo = CeloClient(backend)
    # transaction_params.append("") # ADD DONE PARAMETER
    with celo.sign_transaction_with_rawTx_async(
            ETH_PACKED_DERIVATION_PATH,rawTx
        ):
        navigator.navigate_and_compare(TESTS_ROOT_DIR, test_name, instructions)

    response: bytes = get_async_response(backend)

    assert (response.status == StatusCode.STATUS_OK)

def test_sign_transaction_eip1559(test_name, backend, firmware, navigator):
    if firmware.device == "nanos":
        instructions = get_nano_review_instructions(9)
    elif firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(5)
    else:
        instructions = get_stax_review_instructions_with_warning(1)

    rawTx = "02f8d482a4ec820808839b34b4850fbc63d144830204e094004626a008b1acdc4c74ab51644093b155e59a2380b864ba0876520000000000000000000000000000000000000000000000009458660c5b865f23000000000000000000000000e3b72489968f11c15282514f33df24634440393f000000000000000000000000e3b72489968f11c15282514f33df24634440393fc001a0b0799073a2aa771c5e32b88933ff19982dc30f9e4523fde47137ae504793b880a07014a6e3c32a3b34d4118beb298f2200e858599b5e97766dfaa6fea192cde993"
    sign_transaction_with_rawTx(test_name, backend, navigator, instructions, rawTx)

def test_sign_transaction_cip64(test_name, backend, firmware, navigator):
    if firmware.device == "nanos":
        instructions = get_nano_review_instructions(9)
    elif firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(5)
    else:
        instructions = get_stax_review_instructions_with_warning(1)

    rawTx = "7bf84382aef38084773594008502f2856bb08301688c94da52c9ffebd4d54c94a072776126069d43e74f9e8080c094874069fa1eb16d44d622f2e0ca25eea172369bc1018080"
    sign_transaction_with_rawTx(test_name, backend, navigator, instructions, rawTx)
