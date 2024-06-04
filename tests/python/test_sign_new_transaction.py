from pathlib import Path
from .apps.celo import CeloClient, StatusCode, INS
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

def test_add_cUSD_as_fee_currency(test_name, backend, navigator):
    celo = CeloClient(backend)
    data = "0463555344765de816845861e75a25fca122bb6898b8b1282a000000120000a4ec304402201f8217a6310b78bfe63f40e767b1413e7480403603486f8d3b2cfd4aaa8607470220577c04cc84d1e0ad9d344c73b624e0fc36c63d4a5f33654ab0bcd892b068e7a1"
    encoded_data = bytes.fromhex(data)
    with celo.send_in_chunk_async(
            INS.INS_PROVIDE_ERC20_TOKEN_INFORMATION,
            encoded_data
        ):
        pass

    response: bytes = get_async_response(backend)

    assert (response.status == StatusCode.STATUS_OK)

def test_sign_transaction_cip64(test_name, backend, firmware, navigator):
    test_add_cUSD_as_fee_currency(test_name, backend, navigator)

    if firmware.device == "nanos":
        instructions = get_nano_review_instructions(7)
    elif firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(4)
    else:
        instructions = get_stax_review_instructions(1)

    rawTx =  "7bf84382a4ec8084773594008503a11f9db58301688c94da52c9ffebd4d54c94a072776126069d43e74f9e8080c094765DE816845861E75A25FCA122BB6898B8B1282A018080"
    sign_transaction_with_rawTx(test_name, backend, navigator, instructions, rawTx)