from pathlib import Path
from .apps.celo import CeloClient, StatusCode, INS
from .apps.celo_utils import ETH_PACKED_DERIVATION_PATH
from .utils import (
    get_async_response,
    get_nano_review_instructions,
    get_stax_review_instructions,
    get_stax_review_instructions_with_warning,
)
from ragger.navigator import NavInsID, NavIns

import pytest

TESTS_ROOT_DIR = Path(__file__).parent

# Transaction types
TX_TYPE_EIP1559 = 2
TX_TYPE_CIP64 = 123


# @pytest.mark.active_test_scope
def sign_transaction_with_rawTx(
    test_name, backend, scenario_navigator, navigator, rawTx, instruction=[]
):
    celo = CeloClient(backend)
    # For nano devices, we need to navigate and compare the instructions
    if instruction != []:
        with celo.sign_transaction_with_rawTx_async(ETH_PACKED_DERIVATION_PATH, rawTx):
            navigator.navigate_and_compare(TESTS_ROOT_DIR, test_name, instruction)
    else:
        with celo.sign_transaction_with_rawTx_async(ETH_PACKED_DERIVATION_PATH, rawTx):
            scenario_navigator.review_approve(TESTS_ROOT_DIR, test_name)

    response: bytes = get_async_response(backend)
    return response


# @pytest.mark.active_test_scope
def test_sign_transaction_eip1559_no_data(
    test_name, backend, scenario_navigator, navigator, firmware
):

    rawTx = "02f86c82aef380830f42408506fc35fb8082520894da52c9ffebd4d54c94a072776126069d43e74f9e8080c080a099059ce0f1fe1f4fe27a583a6fd6a12274780d358f332d6e5901953900b8fb22a046ce6d625369fdc8a521c22793d188afbf61500cd3095fc09b761b518560f101"
    print("KM -- firmware: ", firmware)
    instruction = get_nano_review_instructions(4) if firmware.is_nano else []
    response = sign_transaction_with_rawTx(
        test_name,
        backend,
        scenario_navigator,
        navigator,
        rawTx,
        instruction,
    )

    assert response.status == StatusCode.STATUS_OK
    assert response.data[0] == 0x01 or response.data[0] == 0x00
    assert (
        response.data.hex()
        == "014bef650dcad77df8f5cb9385fca75612d3ba5abd1cae4781f1970e67a0ca19af3a4c9485d7e006c45e6907a698fd9cb1b77b162ad1f93d038985a897b32e0db9"
    )


# @pytest.mark.active_test_scope
def test_sign_transaction_eip1559_with_data(backend):
    rawTx = "02f8d482a4ec820808839b34b4850fbc63d144830204e094004626a008b1acdc4c74ab51644093b155e59a2380b864ba0876520000000000000000000000000000000000000000000000009458660c5b865f23000000000000000000000000e3b72489968f11c15282514f33df24634440393f000000000000000000000000e3b72489968f11c15282514f33df24634440393fc001a0b0799073a2aa771c5e32b88933ff19982dc30f9e4523fde47137ae504793b880a07014a6e3c32a3b34d4118beb298f2200e858599b5e97766dfaa6fea192cde993"
    celo = CeloClient(backend)
    with pytest.raises(Exception) as exc_info:  # Expecting the test to fail
        with celo.sign_transaction_with_rawTx_async(ETH_PACKED_DERIVATION_PATH, rawTx):
            pass
    assert "6a80" in str(exc_info.value), "Expected exception to contain '6a80'"


# @pytest.mark.active_test_scope
def test_add_cUSD_as_fee_currency(backend):
    celo = CeloClient(backend)
    data = "0463555344765de816845861e75a25fca122bb6898b8b1282a000000120000a4ec304402201f8217a6310b78bfe63f40e767b1413e7480403603486f8d3b2cfd4aaa8607470220577c04cc84d1e0ad9d344c73b624e0fc36c63d4a5f33654ab0bcd892b068e7a1"
    encoded_data = bytes.fromhex(data)
    with celo.send_in_chunk_async(
        INS.INS_PROVIDE_ERC20_TOKEN_INFORMATION, encoded_data
    ):
        pass

    response: bytes = get_async_response(backend)
    assert response.status == StatusCode.STATUS_OK


# @pytest.mark.active_test_scope
def test_sign_transaction_cip64(
    test_name, backend, scenario_navigator, navigator, firmware
):
    test_add_cUSD_as_fee_currency(backend)

    rawTx = "7bf84382a4ec8084773594008503a11f9db58301688c94da52c9ffebd4d54c94a072776126069d43e74f9e8080c094765DE816845861E75A25FCA122BB6898B8B1282A018080"
    instruction = get_nano_review_instructions(4) if firmware.is_nano else []
    response = sign_transaction_with_rawTx(
        test_name, backend, scenario_navigator, navigator, rawTx, instruction
    )
    assert response.status == StatusCode.STATUS_OK
    assert response.data[0] == 0x01 or response.data[0] == 0x00
    assert (
        response.data.hex()
        == "015b39e09e1ef6dd34d140cb3ee9e621c0de28aa0fd965b6b843a1838bdab18d855bb7c01e103beaac3ee3d76fa80bf6167796a33890c8226ac2ccd7ad844a41ab"
    )
