from pathlib import Path
from .apps.celo import CeloClient, StatusCode, INS
from .apps.celo_utils import ETH_PACKED_DERIVATION_PATH, CELO_PACKED_DERIVATION_PATH
from .utils import get_async_response, get_nano_review_instructions, get_stax_review_instructions, get_enable_blind_sign_and_return_review_instructions, get_reject_disabled_blind_signing_screen_instructions, get_go_settings_disabled_blind_signing_screen_instructions, get_nano_enable_blind_sign_and_review_instructions, get_nano_go_settings_disabled_blind_signing_screen_instructions, get_nano_reject_disabled_blind_signing_screen_instructions
from .settings import settings_toggle, SettingID

import threading

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

# "cUSD" descriptor for the same cUSD contract, signed for use as a fee currency.
CUSD_FEE_CURRENCY_TOKEN_DATA = "0463555344765de816845861e75a25fca122bb6898b8b1282a000000120000a4ec304402201f8217a6310b78bfe63f40e767b1413e7480403603486f8d3b2cfd4aaa8607470220577c04cc84d1e0ad9d344c73b624e0fc36c63d4a5f33654ab0bcd892b068e7a1"


def test_add_cUSD_as_fee_currency(test_name, backend, navigator):
    celo = CeloClient(backend)
    encoded_data = bytes.fromhex(CUSD_FEE_CURRENCY_TOKEN_DATA)
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


def build_eip1559_tx(data: bytes, to: bytes = USDT_CONTRACT, value: int = 0,
                     chain_id: int = CELO_CHAIN_ID) -> str:
    nonce = 0x47
    max_priority_fee_per_gas = 0x0f4240
    max_fee_per_gas = 0x060db88400
    gas_limit = 0x01cf08
    access_list = []
    fields = [chain_id, nonce, max_priority_fee_per_gas, max_fee_per_gas,
              gas_limit, to, value, data, access_list]
    return (bytes([TX_TYPE_EIP1559]) + rlp.encode(fields)).hex()


def approval_navigation(backend):
    """Returns what it takes to walk an ordinary clear-signed review to its approval.

    That is the instruction that advances one page, the instructions that approve, and the
    text that marks the approval screen.
    """
    if backend.device.is_nano:
        return NavInsID.RIGHT_CLICK, [NavInsID.BOTH_CLICK], "Accept"
    return (NavInsID.SWIPE_CENTER_TO_LEFT,
            [NavInsID.USE_CASE_REVIEW_CONFIRM, NavInsID.USE_CASE_STATUS_DISMISS],
            "Sign Transaction?")


def navigate_to_approval_and_compare(test_name, backend, navigator):
    """Navigates a clear-signed review to its approval screen, looking for it by name.

    The number of review screens depends on the comment length and on the device, so these
    tests must not count clicks to the approval screen. This only drives the ordinary
    review: the blind-signing flow opens on a choice page that no repeated navigation
    instruction can get past.
    """
    navigate_instruction, validation_instructions, approval_text = approval_navigation(backend)

    navigator.navigate_until_text_and_compare(navigate_instruction,
                                              validation_instructions,
                                              approval_text,
                                              TESTS_ROOT_DIR,
                                              test_name)


def provision_usdt_and_sign(test_name, backend, navigator, comment_data):
    """Signs a transferWithComment payload, navigating to the approval screen by name.

    A comment changes how many review screens there are, and by how much per device, so
    these tests look for the approval screen instead of counting clicks to it.
    """
    celo = CeloClient(backend)
    response = celo.provide_token_information(bytes.fromhex(USDT_TOKEN_DATA))
    assert response.status == StatusCode.STATUS_OK

    rawTx = build_eip1559_tx(comment_data)
    with celo.sign_transaction_with_rawTx_async(CELO_PACKED_DERIVATION_PATH, rawTx):
        navigate_to_approval_and_compare(test_name, backend, navigator)
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


# Init code for a contract creation; its content is irrelevant, only that it is non-empty
# data signed with an empty destination.
CONTRACT_CREATION_BYTECODE = bytes.fromhex("60806040523480156100105760006000fd5b50")

# selector + to + value: the part of the calldata the parser needs to recognise a token
# transfer. See TOKEN_TRANSFER_DATA_LEN in src/globals.h.
TRANSFER_PREFIX_LEN = 4 + 2 * ABI_WORD_LEN


def test_transfer_refused_when_token_metadata_is_evicted(test_name, backend, navigator):
    """Token metadata taken away after the calldata was classified must refuse.

    Blind signing is enabled on purpose: it removes every other reason to refuse this
    transaction, so a pass here can only come from the evicted-metadata check. Without it
    the transaction is signed with no token amount ever having been resolved.
    """
    settings_toggle(backend.device, navigator, [SettingID.BLIND_SIGNING])

    celo = CeloClient(backend)
    assert celo.provide_token_information(
        bytes.fromhex(USDT_TOKEN_DATA)).status == StatusCode.STATUS_OK

    data = build_transfer_with_comment_data(b"Coffee money")
    payload = CELO_PACKED_DERIVATION_PATH + bytes.fromhex(build_eip1559_tx(data))

    # Split inside the data field, past the selector and the transfer prefix, so the parser
    # has already latched PROVISION_TOKEN when the metadata disappears.
    split = payload.index(TRANSFER_WITH_COMMENT_SELECTOR) + TRANSFER_PREFIX_LEN
    assert celo.send_sign_chunk(payload[:split], first=True).status == StatusCode.STATUS_OK

    # MAX_TOKEN is 2 and the slots are filled round-robin, so two more descriptors
    # overwrite the USDT entry this transaction was classified against.
    assert celo.provide_token_information(
        bytes.fromhex(CELO_DOLLAR_TOKEN_DATA)).status == StatusCode.STATUS_OK
    assert celo.provide_token_information(
        bytes.fromhex(CUSD_FEE_CURRENCY_TOKEN_DATA)).status == StatusCode.STATUS_OK

    with pytest.raises(ragger.error.ExceptionRAPDU) as exc:
        celo.send_sign_chunk(payload[split:], first=False)
    assert exc.value.status == StatusCode.STATUS_ERROR_IN_DATA


def test_contract_creation_warns_even_with_contract_details_enabled(test_name, backend,
                                                                    navigator):
    """Contract creation must show the blind-signing warning whatever the settings say.

    Init code never reaches the per-field review, so the contract-details setting is not
    evidence that it was seen. With both settings on, the review used to look like an
    ordinary clear-signed transfer.
    """
    settings_toggle(backend.device, navigator,
                    [SettingID.BLIND_SIGNING, SettingID.DEBUG_DATA])

    # Same flow as test_sign_transaction_eip1559_no_token_desc_blind_enabled: on the
    # touchscreens the blind-signing warning is a choice page whose second button
    # ("Continue anyway") is USE_CASE_CHOICE_REJECT, so it takes these instructions rather
    # than a repeated swipe.
    if backend.device.is_nano:
        instructions = get_nano_enable_blind_sign_and_review_instructions(4)
    else:
        instructions = get_enable_blind_sign_and_return_review_instructions(2)

    rawTx = build_eip1559_tx(CONTRACT_CREATION_BYTECODE, to=b"")
    response = sign_transaction_with_rawTx_celo(test_name, backend, navigator,
                                                instructions, rawTx)

    assert response.data[0] in (0, 1)
    assert response.status == StatusCode.STATUS_OK


def test_sign_first_chunk_without_tx_type_is_refused(test_name, backend, navigator):
    """A first chunk that stops at the end of the derivation path must be refused.

    The transaction type byte was read from beyond the received data: a stale
    G_io_apdu_buffer byte of 0x02 or 0x7b was accepted as the type, hashed into the
    preimage, and left the remaining length wrapped to 65535 for the parser to read.
    """
    celo = CeloClient(backend)
    with pytest.raises(ragger.error.ExceptionRAPDU) as exc:
        celo.send_sign_chunk(CELO_PACKED_DERIVATION_PATH, first=True)
    assert exc.value.status == StatusCode.STATUS_ERROR_IN_DATA


def test_sign_first_chunk_with_unsupported_tx_type_is_refused(test_name, backend,
                                                              navigator):
    # One byte past the path is enough to reach the type check, so the length guard must
    # not turn an unsupported type into a generic data error.
    celo = CeloClient(backend)
    with pytest.raises(ragger.error.ExceptionRAPDU) as exc:
        celo.send_sign_chunk(CELO_PACKED_DERIVATION_PATH + bytes([0x01]), first=True)
    assert exc.value.status == StatusCode.STATUS_DEPRECATED


def test_sign_first_chunk_with_tx_type_only_is_accepted(test_name, backend, navigator):
    # The smallest legitimate first chunk: path plus the type byte, nothing else. The
    # parser has to be left waiting for the rest rather than refused.
    celo = CeloClient(backend)
    response = celo.send_sign_chunk(CELO_PACKED_DERIVATION_PATH + bytes([TX_TYPE_EIP1559]),
                                    first=True)
    assert response.status == StatusCode.STATUS_OK

# Celo core contracts on mainnet; the staking decode is bound to these, so the
# same calldata sent anywhere else must not be relabelled as an official action.
# An address that is not a Celo core contract.
ARBITRARY_DESTINATION = bytes.fromhex("da52c9ffebd4d54c94a072776126069d43e74f9e")
LOCKED_GOLD_CONTRACT = bytes.fromhex("6cc083aed9e3ebe302a6336dbc7c921c9f03349e")
ELECTION_CONTRACT = bytes.fromhex("8d6677192144292870907e3fa8a5527fe55a7ff6")

WITHDRAW_SELECTOR = bytes.fromhex("2e1a7d4d")
VOTE_SELECTOR = bytes.fromhex("580d747a")


def expect_not_clear_signed(backend, data, to):
    """Asserts a transaction is refused as unrecognised calldata.

    Blind signing is off, so calldata the parser does not recognise is refused
    outright. A transaction that still gets clear-signed would instead reach a
    review and return OK, which is the regression these tests guard.
    """
    celo = CeloClient(backend)
    rawTx = build_eip1559_tx(data, to=to)
    with pytest.raises(ragger.error.ExceptionRAPDU) as exc:
        with celo.sign_transaction_with_rawTx_async(CELO_PACKED_DERIVATION_PATH, rawTx):
            pass
    assert exc.value.status == StatusCode.STATUS_ERROR_IN_DATA


def test_withdraw_selector_on_other_contract_is_not_staking(test_name, backend,
                                                            navigator):
    # withdraw(uint256) is one of the most widely deployed signatures in DeFi --
    # WETH unwraps and countless vaults use it -- so matching on the selector
    # alone labelled any of them as a Celo withdrawal.
    data = WITHDRAW_SELECTOR + encode_abi_word(0)
    expect_not_clear_signed(backend, data, ARBITRARY_DESTINATION)


def test_withdraw_selector_on_election_is_not_staking(test_name, backend,
                                                      navigator):
    # A real core contract, but not the one that implements withdraw.
    data = WITHDRAW_SELECTOR + encode_abi_word(0)
    expect_not_clear_signed(backend, data, ELECTION_CONTRACT)


def test_vote_selector_on_other_contract_is_not_staking(test_name, backend,
                                                        navigator):
    # The payload layout of Election.vote on an attacker-chosen contract: this
    # used to be presented as "Type: Vote" with an address taken from the
    # calldata, and the contract really being called was never displayed.
    data = (VOTE_SELECTOR
            + encode_abi_word(int.from_bytes(TRANSFER_RECIPIENT, byteorder="big"))
            + encode_abi_word(10 ** 18)
            + encode_abi_word(0)
            + encode_abi_word(0))
    expect_not_clear_signed(backend, data, ARBITRARY_DESTINATION)


def test_vote_selector_on_locked_gold_is_not_staking(test_name, backend,
                                                     navigator):
    data = (VOTE_SELECTOR
            + encode_abi_word(int.from_bytes(TRANSFER_RECIPIENT, byteorder="big"))
            + encode_abi_word(10 ** 18)
            + encode_abi_word(0)
            + encode_abi_word(0))
    expect_not_clear_signed(backend, data, LOCKED_GOLD_CONTRACT)


# Native value and network binding of the clear-signed calls.
#
# A clear-signed review replaces the transaction's own value and recipient with the ones
# decoded from calldata, so any native value attached to the transaction is signed without
# being shown. Token metadata is also only meaningful on the network its descriptor was
# signed for.

ALFAJORES_CHAIN_ID = 44787
BAKLAVA_CHAIN_ID = 62320
CELO_SEPOLIA_CHAIN_ID = 11142220

# Contract address inside CUSD_FEE_CURRENCY_TOKEN_DATA.
CUSD_CONTRACT = bytes.fromhex("765de816845861e75a25fca122bb6898b8b1282a")

# "b cUSD" descriptor signed for Baklava (chainId 62320), as published in
# doc/provideERC20TokenInformation.txt.
BAKLAVA_CUSD_TOKEN_DATA = "0662206355534462492a644a588fd904270bed06ad52b9abfea1ae000000120000f370304402200f64d7b8cdef156dc9b4af61df804b4fb0fdcc562f613e386894baa7c52879db02207998a030e8ed6ba4cb3f1796c3731016d605b507e8b1c8df3aa904d8fe7d2149"
BAKLAVA_CUSD_CONTRACT = bytes.fromhex("62492a644a588fd904270bed06ad52b9abfea1ae")

# A contract that no signed descriptor names.
UNSIGNED_TOKEN_CONTRACT = bytes.fromhex("c0ffee254729296a45a3885639ac7e10f9d54979")

ACCOUNTS_CONTRACT = bytes.fromhex("7d21685c17607338b313a7174bab6620bad0aab7")

TRANSFER_SELECTOR = bytes.fromhex("a9059cbb")
LOCK_SELECTOR = bytes.fromhex("f83d08ba")

# Any address, as the group or validator argument of a staking call.
STAKING_ADDRESS_WORD = int.from_bytes(TRANSFER_RECIPIENT, byteorder="big")
STAKING_AMOUNT = 10 ** 18


def build_transfer_data(recipient: bytes = TRANSFER_RECIPIENT, amount: int = 10000) -> bytes:
    return (TRANSFER_SELECTOR
            + encode_abi_word(int.from_bytes(recipient, byteorder="big"))
            + encode_abi_word(amount))


def build_cip64_tx(fee_currency: bytes, chain_id: int = CELO_CHAIN_ID) -> str:
    nonce = 0
    max_priority_fee_per_gas = 0x77359400
    max_fee_per_gas = 0x03a11f9db5
    gas_limit = 0x01688c
    value = 0
    data = b""
    access_list = []
    # yParity, r and s are present but unsigned.
    fields = [chain_id, nonce, max_priority_fee_per_gas, max_fee_per_gas, gas_limit,
              ARBITRARY_DESTINATION, value, data, access_list, fee_currency, 1, b"", b""]
    return (bytes([TX_TYPE_CIP64]) + rlp.encode(fields)).hex()


def staking_call(selector: str, *words: int) -> bytes:
    return bytes.fromhex(selector) + b"".join(encode_abi_word(word) for word in words)


# Every staking call the app clear-signs except lock(), each with the core contract that
# implements it. None of them is payable on that contract.
NONPAYABLE_STAKING_CALLS = {
    "unlock": (LOCKED_GOLD_CONTRACT, staking_call("6198e339", STAKING_AMOUNT)),
    "relock": (LOCKED_GOLD_CONTRACT, staking_call("b2fb30cb", 0, STAKING_AMOUNT)),
    "withdraw": (LOCKED_GOLD_CONTRACT, staking_call("2e1a7d4d", 0)),
    "vote": (ELECTION_CONTRACT,
             staking_call("580d747a", STAKING_ADDRESS_WORD, STAKING_AMOUNT, 0, 0)),
    "activate": (ELECTION_CONTRACT, staking_call("1c5a9d9c", STAKING_ADDRESS_WORD)),
    "revoke_pending": (ELECTION_CONTRACT,
                       staking_call("9dfb6081", STAKING_ADDRESS_WORD, STAKING_AMOUNT, 0, 0, 0)),
    "revoke_active": (ELECTION_CONTRACT,
                      staking_call("6e198475", STAKING_ADDRESS_WORD, STAKING_AMOUNT, 0, 0, 0)),
    "create_account": (ACCOUNTS_CONTRACT, staking_call("9dca362f")),
}

TOKEN_CALLS = {
    "transfer": build_transfer_data(),
    "transferWithComment": build_transfer_with_comment_data(b"Coffee money"),
}

# Chain IDs the app signs for, other than the one the USDT and cUSD descriptors are for.
OTHER_CHAIN_IDS = {
    "alfajores": ALFAJORES_CHAIN_ID,
    "baklava": BAKLAVA_CHAIN_ID,
    "celo_sepolia": CELO_SEPOLIA_CHAIN_ID,
}


# A refusal is answered at once. An accepted transaction opens a review instead, and the
# app does not answer until the user acts.
REFUSAL_TIMEOUT_SECONDS = 10

# Longest the approval screen may take to appear.
APPROVAL_TIMEOUT_SECONDS = 30


def expect_refused(backend, rawTx):
    """Asserts a transaction is refused as soon as it is sent, before any review.

    The exchange of an accepted transaction blocks until the user acts, so it runs in a
    thread whose wait is bounded: that case then fails instead of hanging the run. The
    thread is left blocked and dies with the emulator when the test tears down.
    """
    celo = CeloClient(backend)
    outcome = {}

    def send():
        try:
            with celo.sign_transaction_with_rawTx_async(CELO_PACKED_DERIVATION_PATH, rawTx):
                pass
        except ragger.error.ExceptionRAPDU as exc:
            outcome["status"] = exc.status

    sender = threading.Thread(target=send, daemon=True)
    sender.start()
    sender.join(REFUSAL_TIMEOUT_SECONDS)

    assert not sender.is_alive(), "the app opened a review instead of refusing"
    assert outcome.get("status") == StatusCode.STATUS_ERROR_IN_DATA, \
        f"the app did not refuse with a data error: {outcome}"


def sign_and_expect_approval(backend, navigator, rawTx):
    """Signs a transaction whose review is an ordinary clear-signed one and approves it.

    Blind signing is left off, so reaching the approval screen means the calldata was
    recognised rather than merely tolerated.
    """
    navigate_instruction, validation_instructions, approval_text = approval_navigation(backend)

    celo = CeloClient(backend)
    with celo.sign_transaction_with_rawTx_async(CELO_PACKED_DERIVATION_PATH, rawTx):
        navigator.navigate_until_text(navigate_instruction, validation_instructions,
                                      approval_text, timeout=APPROVAL_TIMEOUT_SECONDS)
    response = get_async_response(backend)
    assert response.status == StatusCode.STATUS_OK


def provide_token(celo, descriptor_hex):
    response = celo.provide_token_information(bytes.fromhex(descriptor_hex))
    assert response.status == StatusCode.STATUS_OK


def forge_descriptor(descriptor_hex: str, contract: bytes) -> bytes:
    """Swaps the contract address of a signed descriptor and keeps its signature.

    The signature covers the address, so the result is well-formed but unsigned for it.
    """
    descriptor = bytes.fromhex(descriptor_hex)
    address_start = 1 + descriptor[0]
    return (descriptor[:address_start] + contract
            + descriptor[address_start + len(contract):])


@pytest.mark.parametrize("data", list(TOKEN_CALLS.values()), ids=list(TOKEN_CALLS))
def test_token_transfer_with_native_value_is_refused(backend, navigator, data):
    """A recognised token call must not carry native value.

    The review would show the token amount and recipient while the transaction also sends
    native CELO to the token contract. Blind signing is on, which takes the blind-signing
    gate out of play: the refusal has to come from the value itself.
    """
    settings_toggle(backend.device, navigator, [SettingID.BLIND_SIGNING])
    provide_token(CeloClient(backend), USDT_TOKEN_DATA)

    expect_refused(backend, build_eip1559_tx(data, value=1))


@pytest.mark.parametrize("contract, data", list(NONPAYABLE_STAKING_CALLS.values()),
                         ids=list(NONPAYABLE_STAKING_CALLS))
def test_staking_call_with_native_value_is_refused(backend, navigator, contract, data):
    """A staking call that is not payable must not carry native value.

    None of these reviews shows the transaction's own value: some replace it with a
    calldata amount and the rest never display an amount at all. Blind signing is on for
    the same reason as for token transfers.
    """
    settings_toggle(backend.device, navigator, [SettingID.BLIND_SIGNING])

    expect_refused(backend, build_eip1559_tx(data, to=contract, value=1))


@pytest.mark.parametrize("contract, data", list(NONPAYABLE_STAKING_CALLS.values()),
                         ids=list(NONPAYABLE_STAKING_CALLS))
def test_staking_call_without_native_value_is_clear_signed(backend, navigator, contract, data):
    # The refusal tests above are only meaningful if these exact calls are recognised.
    sign_and_expect_approval(backend, navigator, build_eip1559_tx(data, to=contract))


def test_lock_keeps_its_native_value(backend, navigator):
    # lock() is payable and its native value is the amount locked, which the review shows.
    rawTx = build_eip1559_tx(LOCK_SELECTOR, to=LOCKED_GOLD_CONTRACT, value=STAKING_AMOUNT)
    sign_and_expect_approval(backend, navigator, rawTx)


@pytest.mark.parametrize("chain_id", list(OTHER_CHAIN_IDS.values()), ids=list(OTHER_CHAIN_IDS))
def test_descriptor_is_not_used_on_another_chain(backend, chain_id):
    """A descriptor signed for mainnet must not label a call on another network.

    The same address can be a different contract, or no contract, on another chain. With
    blind signing off, a call that is not recognised as a token transfer is refused.
    """
    provide_token(CeloClient(backend), USDT_TOKEN_DATA)

    expect_refused(backend, build_eip1559_tx(build_transfer_data(), chain_id=chain_id))


def test_baklava_descriptor_is_not_used_on_mainnet(backend):
    provide_token(CeloClient(backend), BAKLAVA_CUSD_TOKEN_DATA)

    expect_refused(backend, build_eip1559_tx(build_transfer_data(), to=BAKLAVA_CUSD_CONTRACT))


def test_baklava_descriptor_is_used_on_baklava(backend, navigator):
    # Control for the refusals above: a descriptor still works on the chain it names.
    provide_token(CeloClient(backend), BAKLAVA_CUSD_TOKEN_DATA)

    rawTx = build_eip1559_tx(build_transfer_data(), to=BAKLAVA_CUSD_CONTRACT,
                             chain_id=BAKLAVA_CHAIN_ID)
    sign_and_expect_approval(backend, navigator, rawTx)


def test_fee_currency_descriptor_is_not_used_on_another_chain(backend):
    # The fee currency decides how the fee is shown, so it is bound to the network too.
    provide_token(CeloClient(backend), CUSD_FEE_CURRENCY_TOKEN_DATA)

    expect_refused(backend, build_cip64_tx(CUSD_CONTRACT, chain_id=BAKLAVA_CHAIN_ID))


def test_fee_currency_descriptor_is_used_on_its_own_chain(backend, navigator):
    provide_token(CeloClient(backend), CUSD_FEE_CURRENCY_TOKEN_DATA)

    sign_and_expect_approval(backend, navigator, build_cip64_tx(CUSD_CONTRACT))


def test_rejected_descriptor_is_not_trusted(backend):
    """A descriptor whose signature fails must leave no trusted metadata behind.

    The fields of a descriptor are copied into the next slot before its signature is
    checked. MAX_TOKEN is 2 and the slots are filled round-robin, so the third descriptor
    lands in the slot of the first. The forged one carries USDT's signature for a
    different contract, so it is refused; however the app arranges it, that contract
    must stay unknown afterwards.
    """
    celo = CeloClient(backend)
    provide_token(celo, USDT_TOKEN_DATA)
    provide_token(celo, CELO_DOLLAR_TOKEN_DATA)

    forged = forge_descriptor(USDT_TOKEN_DATA, UNSIGNED_TOKEN_CONTRACT)
    with pytest.raises(ragger.error.ExceptionRAPDU) as exc:
        celo.provide_token_information(forged)
    assert exc.value.status == StatusCode.STATUS_ERROR_IN_DATA

    expect_refused(backend, build_eip1559_tx(build_transfer_data(),
                                             to=UNSIGNED_TOKEN_CONTRACT))
