from pathlib import Path
from .apps.celo import CeloClient, StatusCode, INS
from .apps.celo_utils import ETH_PACKED_DERIVATION_PATH, CELO_PACKED_DERIVATION_PATH
from .utils import get_async_response, get_nano_review_instructions, get_stax_review_instructions, get_enable_blind_sign_and_return_review_instructions, get_reject_disabled_blind_signing_screen_instructions, get_go_settings_disabled_blind_signing_screen_instructions, get_nano_enable_blind_sign_and_review_instructions, get_nano_go_settings_disabled_blind_signing_screen_instructions, get_nano_reject_disabled_blind_signing_screen_instructions
from .settings import settings_toggle, SettingID

import pytest
import ragger
import rlp

from ragger.navigator import NavInsID

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


def test_sign_transaction_eip1559_no_data(test_name, backend, navigator):
    if backend.device.is_nano:
        instructions = get_nano_review_instructions(4)
    else:
        instructions = get_stax_review_instructions(1)

    rawTx = "02f86c82aef380830f42408506fc35fb8082520894da52c9ffebd4d54c94a072776126069d43e74f9e8080c080a099059ce0f1fe1f4fe27a583a6fd6a12274780d358f332d6e5901953900b8fb22a046ce6d625369fdc8a521c22793d188afbf61500cd3095fc09b761b518560f101"
    response = sign_transaction_with_rawTx(test_name, backend, navigator, instructions, rawTx)

    assert(response.data[0] == 0x01 or response.data[0] == 0x00)
    assert(response.status == StatusCode.STATUS_OK)


USDT_TOKEN_DATA = "045553445448065fbbe25f71c9282ddf5e1cd6d6a887483d5e000000060000a4ec304402204239b4af138d118b70fa5aea895b5f000a0679077434b2c30257c7a841c027fa02207e76ec1cc7485b4d4545c462b4e4baf8e0fd6ab557019ae974079ab0c51fe28d"


def test_add_tether_usdt_token_clabs_sig(test_name, backend, navigator):
    celo = CeloClient(backend)
    encoded_data = bytes.fromhex(USDT_TOKEN_DATA)
    with celo.send_in_chunk_async(
            INS.INS_PROVIDE_ERC20_TOKEN_INFORMATION,
            encoded_data
        ):
        pass

    response: bytes = get_async_response(backend)
    assert response.status == StatusCode.STATUS_OK
    assert len(response.data) == 1
    assert response.data[0] in (0, 1)


def test_add_tether_usdt_token_ledger_sig(test_name, backend, navigator):
    celo = CeloClient(backend)
    encoded_data = bytes.fromhex(USDT_TOKEN_DATA)
    with celo.send_in_chunk_async(
            INS.INS_PROVIDE_ERC20_TOKEN_INFORMATION,
            encoded_data
        ):
        pass

    response: bytes = get_async_response(backend)
    assert response.status == StatusCode.STATUS_OK
    assert len(response.data) == 1
    assert response.data[0] in (0, 1)


def test_sign_transaction_eip1559_with_data(test_name, backend, navigator):
    test_add_tether_usdt_token_ledger_sig(test_name, backend, navigator)

    if backend.device.is_nano:
        instructions = get_nano_review_instructions(4)
    else:
        instructions = get_stax_review_instructions(1)
    rawTx =  "02f86f82a4ec47830f424085060db884008301cf089448065fbbe25f71c9282ddf5e1cd6d6a887483d5e80b844a9059cbb000000000000000000000000abd5d4575341b5878a1e7cb75dc0d4da91dfafa30000000000000000000000000000000000000000000000000000000000002710c0"

    response = sign_transaction_with_rawTx_celo(test_name, backend, navigator, instructions, rawTx)
    assert(response.data[0] == 0x01 or response.data[0] == 0x00)
    assert(response.status == StatusCode.STATUS_OK)


def test_sign_transaction_eip1559_no_token_desc_blind_enabled(test_name, backend, navigator):
    settings_toggle(backend.device, navigator, [SettingID.BLIND_SIGNING])

    if backend.device.is_nano:
        instructions = get_nano_enable_blind_sign_and_review_instructions(4)
    else:
        instructions = get_enable_blind_sign_and_return_review_instructions(2)
    rawTx =  "02f86f82a4ec47830f424085060db884008301cf089448065fbbe25f71c9282ddf5e1cd6d6a887483d5e80b844a9059cbb000000000000000000000000abd5d4575341b5878a1e7cb75dc0d4da91dfafa30000000000000000000000000000000000000000000000000000000000002710c0"

    response = sign_transaction_with_rawTx_celo(test_name, backend, navigator, instructions, rawTx)
    assert(response.data[0] == 0x01 or response.data[0] == 0x00)
    assert(response.status == StatusCode.STATUS_OK)


def test_try_sign_tx_eip1559_no_token_desc_blind_disabled_rejected(test_name, backend, navigator):
    if backend.device.is_nano:
        instructions = get_nano_reject_disabled_blind_signing_screen_instructions()
    else:
        instructions = get_reject_disabled_blind_signing_screen_instructions()
    rawTx =  "02f86f82a4ec47830f424085060db884008301cf089448065fbbe25f71c9282ddf5e1cd6d6a887483d5e80b844a9059cbb000000000000000000000000abd5d4575341b5878a1e7cb75dc0d4da91dfafa30000000000000000000000000000000000000000000000000000000000002710c0"

    try:
        sign_transaction_with_rawTx_celo(test_name, backend, navigator, instructions, rawTx)
        # assert(response.status == StatusCode.STATUS_ERROR_IN_DATA)
    except ragger.error.ExceptionRAPDU as e:
        assert(e.status == StatusCode.STATUS_ERROR_IN_DATA)

def test_try_sign_tx_eip1559_no_token_desc_blind_disabled_go_settings(test_name, backend, navigator):
    if backend.device.is_nano:
        instructions = get_nano_go_settings_disabled_blind_signing_screen_instructions()
    else:
        instructions = get_go_settings_disabled_blind_signing_screen_instructions()
    rawTx =  "02f86f82a4ec47830f424085060db884008301cf089448065fbbe25f71c9282ddf5e1cd6d6a887483d5e80b844a9059cbb000000000000000000000000abd5d4575341b5878a1e7cb75dc0d4da91dfafa30000000000000000000000000000000000000000000000000000000000002710c0"

    try:
        sign_transaction_with_rawTx_celo(test_name, backend, navigator, instructions, rawTx)
    except ragger.error.ExceptionRAPDU as e:
        assert(e.status == StatusCode.STATUS_ERROR_IN_DATA)

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
    assert response.status == StatusCode.STATUS_OK
    assert len(response.data) == 1
    assert response.data[0] in (0, 1)


def test_sign_transaction_cip64(test_name, backend, navigator):
    test_add_cUSD_as_fee_currency(test_name, backend, navigator)

    if backend.device.is_nano:
        instructions = get_nano_review_instructions(4)
    else:
        instructions = get_stax_review_instructions(1)

    rawTx =  "7bf84382a4ec8084773594008503a11f9db58301688c94da52c9ffebd4d54c94a072776126069d43e74f9e8080c094765DE816845861E75A25FCA122BB6898B8B1282A018080"
    response = sign_transaction_with_rawTx(test_name, backend, navigator, instructions, rawTx)
    assert(response.data[0] == 0x01 or response.data[0] == 0x00)
    assert(response.status == StatusCode.STATUS_OK)


def test_sign_tx_lock_with_data(test_name, backend, navigator):
    if backend.device.is_nano:
        instructions = get_nano_review_instructions(4)
    else:
        instructions = get_stax_review_instructions(1)

    data =  "058000002c8000ce1080000001000000000000000002f682a4ec17830f42408506fc32ee4083063bd0946cc083aed9e3ebe302a6336dbc7c921c9f03349e88016345785d8a000084f83d08bac0"

    celo = CeloClient(backend)
    encoded_data = bytes.fromhex(data)
    with celo.send_in_chunk_async(
            INS.INS_SIGN,
            encoded_data
        ):
        navigator.navigate_and_compare(TESTS_ROOT_DIR, test_name, instructions)

    response: bytes = get_async_response(backend)
    assert (response.status == StatusCode.STATUS_OK)


def test_sign_tx_vote_with_data(test_name, backend, navigator):
    if backend.device.is_nano:
        instructions = get_nano_review_instructions(5)
    elif backend.device.name == "stax":
        instructions = get_stax_review_instructions(1)
    else:
        instructions = get_stax_review_instructions(2)

    data =  "058000002c8000ce1080000001000000000000000002f8af82a4ec18830f42408506fc32ee40831926e8948d6677192144292870907e3fa8a5527fe55a7ff680b884580d747a0000000000000000000000000861a61bf679a30680510ecc238ee43b82c5e84300000000000000000000000000000000000000000000000002c68af0bb1400000000000000000000000000007c75b0b81a54359e9dccda9cb663ca2e3de6b710000000000000000000000000d72ed2e3db984bac3bb351fe652200de527effcfc0"

    celo = CeloClient(backend)
    encoded_data = bytes.fromhex(data)
    with celo.send_in_chunk_async(
            INS.INS_SIGN,
            encoded_data
        ):
        navigator.navigate_and_compare(TESTS_ROOT_DIR, test_name, instructions)

    response: bytes = get_async_response(backend)
    assert (response.status == StatusCode.STATUS_OK)


def test_sign_tx_unlock_with_data(test_name, backend, navigator):
    if backend.device.is_nano:
        instructions = get_nano_review_instructions(4)
    else:
        instructions = get_stax_review_instructions(1)

    data =  "058000002c8000ce1080000001000000000000000002f84e82a4ec18830f42408506fc32ee4083432c08946cc083aed9e3ebe302a6336dbc7c921c9f03349e80a46198e3390000000000000000000000000000000000000000000000000de0b6b3a7640000c0"

    celo = CeloClient(backend)
    encoded_data = bytes.fromhex(data)
    with celo.send_in_chunk_async(
            INS.INS_SIGN,
            encoded_data
        ):
        navigator.navigate_and_compare(TESTS_ROOT_DIR, test_name, instructions)

    response: bytes = get_async_response(backend)
    assert (response.status == StatusCode.STATUS_OK)


def test_sign_tx_withdraw(test_name, backend, navigator):
    if backend.device.is_nano:
        instructions = get_nano_review_instructions(3)
    else:
        instructions = get_stax_review_instructions(1)

    # EIP-1559 withdraw(uint256 index=0) on Celo, path 44'/52752'/1'/0/0
    # Same structure as unlock; selector 2e1a7d4d triggers PROVISION_WITHDRAW.
    data = "058000002c8000ce1080000001000000000000000002f84e82a4ec18830f42408506fc32ee4083432c08946cc083aed9e3ebe302a6336dbc7c921c9f03349e80a42e1a7d4d0000000000000000000000000000000000000000000000000000000000000000c0"

    celo = CeloClient(backend)
    encoded_data = bytes.fromhex(data)
    with celo.send_in_chunk_async(
            INS.INS_SIGN,
            encoded_data
        ):
        navigator.navigate_and_compare(TESTS_ROOT_DIR, test_name, instructions)

    response: bytes = get_async_response(backend)
    assert (response.status == StatusCode.STATUS_OK)


def test_sign_tx_create_account(test_name, backend, navigator):
    if backend.device.is_nano:
        instructions = get_nano_review_instructions(3)
    else:
        instructions = get_stax_review_instructions(1)

    # EIP-1559 createAccount() on Celo (chainId 42220), path 44'/60'/0'/0'/0'
    # Reproduces the 6a80 bug: selector 9dca362f matched as PROVISION_CREATE_ACCOUNT
    # but finalizeParsing() fell through to the blind-signing guard before the fix.
    data = "058000002c8000003c80000000800000008000000002ee82a4ec80830f42408537ed77e359830441c4947d21685c17607338b313a7174bab6620bad0aab780849dca362fc0"

    celo = CeloClient(backend)
    encoded_data = bytes.fromhex(data)
    with celo.send_in_chunk_async(
            INS.INS_SIGN,
            encoded_data
        ):
        navigator.navigate_and_compare(TESTS_ROOT_DIR, test_name, instructions)

    response: bytes = get_async_response(backend)
    assert (response.status == StatusCode.STATUS_OK)
# "Celo Dollar" token descriptor payload (11-char ticker, cUSD address, decimals=18, chainId=42220).
# Previously rejected by ticker[10] (11+2 >= 10); accepted after expanding to ticker[51].
CELO_DOLLAR_TOKEN_DATA = "0b43656c6f20446f6c6c6172765de816845861e75a25fca122bb6898b8b1282a000000120000a4ec30440220071b4e984b14f78b96ec3d8d7c294b8b18e72ee966b1c299f6f66a2dea30345002200fc9cd21d86c3e328c25cbc376882a00d1ef1a834f69ad4fdcebbbf7c493b2c6"


def test_add_celo_dollar_token(test_name, backend, navigator):
    celo = CeloClient(backend)
    response = celo.provide_token_information(bytes.fromhex(CELO_DOLLAR_TOKEN_DATA))
    assert response.status == StatusCode.STATUS_OK
    assert len(response.data) == 1
    assert response.data[0] in (0, 1)


def test_sign_transaction_with_celo_dollar_token(test_name, backend, navigator):
    celo = CeloClient(backend)
    data = bytes.fromhex(CELO_DOLLAR_TOKEN_DATA)
    response = celo.provide_token_information(data)
    assert response.status == StatusCode.STATUS_OK

    if backend.device.is_nano:
        instructions = get_nano_review_instructions(4)
    else:
        instructions = get_stax_review_instructions(1)

    # EIP-1559 transfer of 10000 base units to the cUSD contract (same structure as USDT test)
    rawTx = "02f86f82a4ec47830f424085060db884008301cf0894765de816845861e75a25fca122bb6898b8b1282a80b844a9059cbb000000000000000000000000abd5d4575341b5878a1e7cb75dc0d4da91dfafa30000000000000000000000000000000000000000000000000000000000002710c0"
    response = sign_transaction_with_rawTx_celo(test_name, backend, navigator, instructions, rawTx)
    assert response.data[0] in (0, 1)
    assert response.status == StatusCode.STATUS_OK


def test_provide_erc20_returns_token_index(test_name, backend, navigator):
    celo = CeloClient(backend)
    response = celo.provide_token_information(bytes.fromhex(USDT_TOKEN_DATA))
    assert response.status == StatusCode.STATUS_OK
    assert len(response.data) == 1
    assert response.data[0] in (0, 1)


def test_provide_erc20_rejects_nonzero_p1(test_name, backend, navigator):
    celo = CeloClient(backend)
    with pytest.raises(ragger.error.ExceptionRAPDU) as exc:
        celo.provide_token_information(bytes.fromhex(USDT_TOKEN_DATA), p1=0x01)
    assert exc.value.status == StatusCode.STATUS_WRONG_P1_OR_P2


def test_provide_erc20_rejects_nonzero_p2(test_name, backend, navigator):
    celo = CeloClient(backend)
    with pytest.raises(ragger.error.ExceptionRAPDU) as exc:
        celo.provide_token_information(bytes.fromhex(USDT_TOKEN_DATA), p2=0x01)
    assert exc.value.status == StatusCode.STATUS_WRONG_P1_OR_P2



# transferWithComment(address to, uint256 value, string comment) — the Celo token method
# that carries a payment memo alongside the transfer.
TRANSFER_WITH_COMMENT_SELECTOR = bytes.fromhex("e1d6aceb")

CELO_CHAIN_ID = 42220
# Contract address inside USDT_TOKEN_DATA, i.e. the token the descriptor above provisions.
USDT_CONTRACT = bytes.fromhex("48065fbbe25f71c9282ddf5e1cd6d6a887483d5e")
TRANSFER_RECIPIENT = bytes.fromhex("abd5d4575341b5878a1e7cb75dc0d4da91dfafa3")

ABI_WORD_LEN = 32
# The comment is the third of three arguments, so the ABI fixes where its bytes start:
# to (32) + value (32) + the offset word itself (32).
ABI_STRING_OFFSET = 96
# Longest comment the app displays in full; see TOKEN_COMMENT_MAX_LEN in src/globals.h.
COMMENT_MAX_LEN = 64


def encode_abi_word(value: int) -> bytes:
    return value.to_bytes(ABI_WORD_LEN, byteorder="big")


def build_transfer_with_comment_data(comment: bytes,
                                     recipient: bytes = TRANSFER_RECIPIENT,
                                     amount: int = 10000,
                                     string_offset: int = ABI_STRING_OFFSET,
                                     declared_length: int = None,
                                     padding_len: int = None) -> bytes:
    """Builds transferWithComment calldata.

    string_offset, declared_length and padding_len are overridable so that tests can
    produce the malformed encodings the signer is required to refuse.
    """
    if declared_length is None:
        declared_length = len(comment)
    if padding_len is None:
        padding_len = (-len(comment)) % ABI_WORD_LEN
    return (TRANSFER_WITH_COMMENT_SELECTOR
            + encode_abi_word(int.from_bytes(recipient, byteorder="big"))
            + encode_abi_word(amount)
            + encode_abi_word(string_offset)
            + encode_abi_word(declared_length)
            + comment
            + bytes(padding_len))


def build_eip1559_tx(data: bytes, to: bytes = USDT_CONTRACT, value: int = 0) -> str:
    nonce = 0x47
    max_priority_fee_per_gas = 0x0f4240
    max_fee_per_gas = 0x060db88400
    gas_limit = 0x01cf08
    access_list = []
    fields = [CELO_CHAIN_ID, nonce, max_priority_fee_per_gas, max_fee_per_gas,
              gas_limit, to, value, data, access_list]
    return (bytes([TX_TYPE_EIP1559]) + rlp.encode(fields)).hex()


def provision_usdt_and_sign(test_name, backend, navigator, comment_data):
    """Signs a transferWithComment payload, navigating to the approval screen by name.

    A comment changes how many review screens there are, and by how much per device, so
    these tests look for the approval screen instead of counting clicks to it.
    """
    celo = CeloClient(backend)
    response = celo.provide_token_information(bytes.fromhex(USDT_TOKEN_DATA))
    assert response.status == StatusCode.STATUS_OK

    if backend.device.is_nano:
        navigate_instruction = NavInsID.RIGHT_CLICK
        validation_instructions = [NavInsID.BOTH_CLICK]
        approval_text = "Accept"
    else:
        navigate_instruction = NavInsID.SWIPE_CENTER_TO_LEFT
        validation_instructions = [NavInsID.USE_CASE_REVIEW_CONFIRM,
                                   NavInsID.USE_CASE_STATUS_DISMISS]
        approval_text = "Sign Transaction?"

    rawTx = build_eip1559_tx(comment_data)
    with celo.sign_transaction_with_rawTx_async(CELO_PACKED_DERIVATION_PATH, rawTx):
        navigator.navigate_until_text_and_compare(navigate_instruction,
                                                  validation_instructions,
                                                  approval_text,
                                                  TESTS_ROOT_DIR,
                                                  test_name)
    return get_async_response(backend)


def provision_usdt_and_expect_refusal(backend, comment_data):
    celo = CeloClient(backend)
    response = celo.provide_token_information(bytes.fromhex(USDT_TOKEN_DATA))
    assert response.status == StatusCode.STATUS_OK

    rawTx = build_eip1559_tx(comment_data)
    with pytest.raises(ragger.error.ExceptionRAPDU) as exc:
        with celo.sign_transaction_with_rawTx_async(CELO_PACKED_DERIVATION_PATH, rawTx):
            pass
    assert exc.value.status == StatusCode.STATUS_ERROR_IN_DATA


def test_sign_transfer_with_comment_displays_comment(test_name, backend, navigator):
    # The comment is signed, so the review must show it.
    data = build_transfer_with_comment_data(b"Coffee money")
    response = provision_usdt_and_sign(test_name, backend, navigator, data)

    assert response.data[0] in (0, 1)
    assert response.status == StatusCode.STATUS_OK


def test_sign_transfer_with_empty_comment(test_name, backend, navigator):
    # An empty comment is well-formed and has nothing to show, so the review carries no
    # Comment field at all.
    data = build_transfer_with_comment_data(b"")
    response = provision_usdt_and_sign(test_name, backend, navigator, data)

    assert response.data[0] in (0, 1)
    assert response.status == StatusCode.STATUS_OK


def test_sign_transfer_with_max_length_comment(test_name, backend, navigator):
    # The longest comment the app accepts; it spans several pages on the small screens.
    data = build_transfer_with_comment_data(b"C" * COMMENT_MAX_LEN)
    response = provision_usdt_and_sign(test_name, backend, navigator, data)

    assert response.data[0] in (0, 1)
    assert response.status == StatusCode.STATUS_OK


def test_transfer_with_oversized_comment_is_refused(test_name, backend, navigator):
    # Regression test for the unbounded copy into tokenContext.data: this payload used to
    # write 200 bytes of attacker-controlled data into a 68-byte buffer.
    provision_usdt_and_expect_refusal(backend, build_transfer_with_comment_data(b"A" * 200))


def test_transfer_with_comment_one_byte_too_long_is_refused(test_name, backend, navigator):
    provision_usdt_and_expect_refusal(
        backend, build_transfer_with_comment_data(b"B" * (COMMENT_MAX_LEN + 1)))


def test_transfer_with_non_printable_comment_is_refused(test_name, backend, navigator):
    # A comment that cannot be rendered as its own bytes cannot be clear-signed.
    provision_usdt_and_expect_refusal(backend, build_transfer_with_comment_data(b"Pay\x1b[2Jme"))


def test_transfer_with_embedded_nul_comment_is_refused(test_name, backend, navigator):
    # A NUL would end the displayed string while the bytes after it stay in the preimage.
    provision_usdt_and_expect_refusal(backend, build_transfer_with_comment_data(b"Pay\x00hidden"))


def test_transfer_with_non_canonical_string_offset_is_refused(test_name, backend, navigator):
    provision_usdt_and_expect_refusal(
        backend, build_transfer_with_comment_data(b"Coffee", string_offset=128))


def test_transfer_with_understated_comment_length_is_refused(test_name, backend, navigator):
    # Declared length shorter than the bytes actually present: the tail would be signed
    # without being shown.
    provision_usdt_and_expect_refusal(
        backend, build_transfer_with_comment_data(b"C" * 40, declared_length=8))


def test_transfer_with_overstated_comment_length_is_refused(test_name, backend, navigator):
    provision_usdt_and_expect_refusal(
        backend, build_transfer_with_comment_data(b"C" * 8, declared_length=40))


def test_transfer_with_unpadded_comment_is_refused(test_name, backend, navigator):
    # The ABI pads the string to a 32-byte multiple; a short field means undeclared bytes.
    provision_usdt_and_expect_refusal(
        backend, build_transfer_with_comment_data(b"C" * 8, padding_len=0))


def test_transfer_with_comment_length_beyond_field_is_refused(test_name, backend, navigator):
    # Calldata that stops at the end of the ABI header while declaring a comment: the
    # declared length has to be checked even though no comment bytes follow.
    provision_usdt_and_expect_refusal(
        backend, build_transfer_with_comment_data(b"", declared_length=1000))
