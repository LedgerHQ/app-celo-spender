from pathlib import Path
from .apps.celo import CeloClient, StatusCode, INS
from .apps.celo_utils import ETH_PACKED_DERIVATION_PATH, CELO_PACKED_DERIVATION_PATH
from .utils import get_async_response, get_nano_review_instructions, get_stax_review_instructions

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
    return response

def sign_transaction_with_rawTx_celo(test_name, backend, navigator, instructions, rawTx):
    celo = CeloClient(backend)
    # transaction_params.append("") # ADD DONE PARAMETER
    with celo.sign_transaction_with_rawTx_async(
            CELO_PACKED_DERIVATION_PATH, rawTx
        ):
        navigator.navigate_and_compare(TESTS_ROOT_DIR, test_name, instructions)

    response: bytes = get_async_response(backend)
    return response

def test_sign_transaction_eip1559_no_data(test_name, backend, firmware, navigator):
    if firmware.device == "nanos":
        instructions = get_nano_review_instructions(6)
    elif firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(4)
    else:
        instructions = get_stax_review_instructions(1)

    rawTx = "02f86c82aef380830f42408506fc35fb8082520894da52c9ffebd4d54c94a072776126069d43e74f9e8080c080a099059ce0f1fe1f4fe27a583a6fd6a12274780d358f332d6e5901953900b8fb22a046ce6d625369fdc8a521c22793d188afbf61500cd3095fc09b761b518560f101"
    response = sign_transaction_with_rawTx(test_name, backend, navigator, instructions, rawTx)

    assert(response.data[0] == 0x01 or response.data[0] == 0x00)
    assert(response.status == StatusCode.STATUS_OK)

def test_add_tether_usdt_token_clabs_sig(test_name, backend, navigator):
    celo = CeloClient(backend)
    data = "045553445448065fbbe25f71c9282ddf5e1cd6d6a887483d5e000000060000a4ec304402204239b4af138d118b70fa5aea895b5f000a0679077434b2c30257c7a841c027fa02207e76ec1cc7485b4d4545c462b4e4baf8e0fd6ab557019ae974079ab0c51fe28d"
    encoded_data = bytes.fromhex(data)
    with celo.send_in_chunk_async(
            INS.INS_PROVIDE_ERC20_TOKEN_INFORMATION,
            encoded_data
        ):
        pass

    response: bytes = get_async_response(backend)
    assert (response.status == StatusCode.STATUS_OK)

def test_add_tether_usdt_token_ledger_sig(test_name, backend, navigator):
    celo = CeloClient(backend)
    data = "045553445448065fbbe25f71c9282ddf5e1cd6d6a887483d5e000000060000a4ec304402204239b4af138d118b70fa5aea895b5f000a0679077434b2c30257c7a841c027fa02207e76ec1cc7485b4d4545c462b4e4baf8e0fd6ab557019ae974079ab0c51fe28d"
    encoded_data = bytes.fromhex(data)
    with celo.send_in_chunk_async(
            INS.INS_PROVIDE_ERC20_TOKEN_INFORMATION,
            encoded_data
        ):
        pass

    response: bytes = get_async_response(backend)
    assert (response.status == StatusCode.STATUS_OK)

def test_sign_transaction_eip1559_with_data(test_name, backend, firmware, navigator):
    test_add_tether_usdt_token_ledger_sig(test_name, backend, navigator)

    if firmware.device == "nanos":
        instructions = get_nano_review_instructions(6)
    elif firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(4)
    else:
        instructions = get_stax_review_instructions(1)
    rawTx =  "02f86f82a4ec47830f424085060db884008301cf089448065fbbe25f71c9282ddf5e1cd6d6a887483d5e80b844a9059cbb000000000000000000000000abd5d4575341b5878a1e7cb75dc0d4da91dfafa30000000000000000000000000000000000000000000000000000000000002710c0"

    response = sign_transaction_with_rawTx_celo(test_name, backend, navigator, instructions, rawTx)
    assert(response.data[0] == 0x01 or response.data[0] == 0x00)
    assert(response.status == StatusCode.STATUS_OK)

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
    response = sign_transaction_with_rawTx(test_name, backend, navigator, instructions, rawTx)
    assert(response.data[0] == 0x01 or response.data[0] == 0x00)
    assert(response.status == StatusCode.STATUS_OK)
