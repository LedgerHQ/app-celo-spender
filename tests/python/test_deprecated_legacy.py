# from pathlib import Path
# from .apps.celo import CeloClient, StatusCode
# from .apps.celo_utils import CELO_PACKED_DERIVATION_PATH
# from .utils import get_async_response, get_nano_review_instructions, get_stax_review_instructions, get_stax_review_instructions_with_warning

# TESTS_ROOT_DIR = Path(__file__).parent

# TOKEN_TRANSFER_ID = 0xa9059cbb
# TOKEN_TRANSFER_WITH_COMMENT_ID = 0xe1d6aceb
# LOCK_METHOD_ID = 0xf83d08ba
# VOTE_METHOD_ID = 0x580d747a
# ACTIVATE_METHOD_ID = 0x1c5a9d9c
# REVOKE_PENDING_METHOD_ID = 0x9dfb6081
# REVOKE_ACTIVE_METHOD_ID = 0x6e198475
# UNLOCK_METHOD_ID = 0x6198e339
# WITHDRAW_METHOD_ID = 0x2e1a7d4d
# RELOCK_METHOD_ID = 0xb2fb30cb
# CREATE_ACCOUNT_METHOD_ID = 0x9dca362f


# def sign_transaction(test_name, backend, navigator, instructions, payload):
#     celo = CeloClient(backend)
#     try:
#         with celo.sign_transaction_async(CELO_PACKED_DERIVATION_PATH,
#                 [
#                     "1234",                  #Nonce
#                     "1234",                  #GasPrice
#                     "1234",                  #StartGas
#                     "",                      #FeeCurrency
#                     "12345678901234567890",  #GatewayTo
#                     "1234",                  #GatewayFee 
#                     "12345678901234567890",  #To
#                     "",                      #Value
#                     payload,
#                     "",                      #V
#                     "",                      #R
#                     "",                      #S
#                     ""                       #Done
#                     ]
#                 ):
#             pass
#     except Exception as e:
#         assert e.status == StatusCode.STATUS_DEPRECATED


# def sign_transaction_no_gtw(test_name, backend, navigator, instructions, payload):
#     celo = CeloClient(backend)
#     try:
#         with celo.sign_transaction_async(CELO_PACKED_DERIVATION_PATH,
#                 [
#                     "1234",                  #Nonce
#                     "1234",                  #GasPrice
#                     "1234",                  #StartGas
#                     "",                      #FeeCurrency
#                     "",                      #GatewayTo
#                     "1234",                  #GatewayFee 
#                     "12345678901234567890",  #To
#                     "",                      #Value
#                     payload,
#                     "",                      #V
#                     "",                      #R
#                     "",                      #S
#                     ""                       #Done
#                     ]
#                 ):
#                 pass
#     except Exception as e:
#         assert e.status == StatusCode.STATUS_DEPRECATED

# def test_sign_transaction_empty(test_name, backend, firmware, navigator):
#     if firmware.device == "nanos":
#         instructions = get_nano_review_instructions(14)
#     elif firmware.device.startswith("nano"):
#         instructions = get_nano_review_instructions(6)
#     else:
#         instructions = get_stax_review_instructions(2)
#     payload = ""

#     sign_transaction(test_name, backend, navigator, instructions, payload)


# def test_sign_transaction_token_transfer(test_name, backend, firmware, navigator):
#     if firmware.device == "nanos":
#         instructions = get_nano_review_instructions(15)
#     elif firmware.device.startswith("nano"):
#         instructions = get_nano_review_instructions(7)
#     else:
#         instructions = get_stax_review_instructions_with_warning(2)
#     payload = TOKEN_TRANSFER_ID.to_bytes(4, byteorder='big') + b'00' * 32

#     sign_transaction(test_name, backend, navigator, instructions, payload)


# def test_sign_transaction_lock(test_name, backend, firmware, navigator):
#     if firmware.device == "nanos":
#         instructions = get_nano_review_instructions(6)
#     elif firmware.device.startswith("nano"):
#         instructions = get_nano_review_instructions(4)
#     else:
#         instructions = get_stax_review_instructions(1)
#     payload = LOCK_METHOD_ID.to_bytes(4, byteorder='big')

#     sign_transaction(test_name, backend, navigator, instructions, payload)


# def test_sign_transaction_vote(test_name, backend, firmware, navigator):
#     if firmware.device == "nanos":
#         instructions = get_nano_review_instructions(14)
#     elif firmware.device.startswith("nano"):
#         instructions = get_nano_review_instructions(6)
#     else:
#         instructions = get_stax_review_instructions(2)
#     payload = VOTE_METHOD_ID.to_bytes(4, byteorder='big') + b'00' * 64

#     sign_transaction(test_name, backend, navigator, instructions, payload)


# def test_sign_transaction_activate(test_name, backend, firmware, navigator):
#     if firmware.device == "nanos":
#         instructions = get_nano_review_instructions(8)
#     elif firmware.device.startswith("nano"):
#         instructions = get_nano_review_instructions(4)
#     else:
#         instructions = get_stax_review_instructions(1)
#     payload = ACTIVATE_METHOD_ID.to_bytes(4, byteorder='big') + b'00' * 16

#     sign_transaction(test_name, backend, navigator, instructions, payload)


# def test_sign_transaction_revoke(test_name, backend, firmware, navigator):
#     if firmware.device == "nanos":
#         instructions = get_nano_review_instructions(14)
#     elif firmware.device.startswith("nano"):
#         instructions = get_nano_review_instructions(6)
#     else:
#         instructions = get_stax_review_instructions(2)
#     payload = REVOKE_PENDING_METHOD_ID.to_bytes(4, byteorder='big') + b'00' * 80

#     sign_transaction(test_name, backend, navigator, instructions, payload)


# def test_sign_transaction_unlock(test_name, backend, firmware, navigator):
#     if firmware.device == "nanos":
#         instructions = get_nano_review_instructions(11)
#     elif firmware.device.startswith("nano"):
#         instructions = get_nano_review_instructions(5)
#     else:
#         instructions = get_stax_review_instructions(2)
#     payload = UNLOCK_METHOD_ID.to_bytes(4, byteorder='big') + b'00' * 16

#     sign_transaction(test_name, backend, navigator, instructions, payload)


# def test_sign_transaction_withdraw(test_name, backend, firmware, navigator):
#     if firmware.device == "nanos":
#         instructions = get_nano_review_instructions(5)
#     elif firmware.device.startswith("nano"):
#         instructions = get_nano_review_instructions(3)
#     else:
#         instructions = get_stax_review_instructions(1)
#     payload = WITHDRAW_METHOD_ID.to_bytes(4, byteorder='big') + b'00' * 16

#     sign_transaction(test_name, backend, navigator, instructions, payload)


# def test_sign_transaction_relock(test_name, backend, firmware, navigator):
#     if firmware.device == "nanos":
#         instructions = get_nano_review_instructions(11)
#     elif firmware.device.startswith("nano"):
#         instructions = get_nano_review_instructions(5)
#     else:
#         instructions = get_stax_review_instructions(2)
#     payload = RELOCK_METHOD_ID.to_bytes(4, byteorder='big') + b'00' * 32

#     sign_transaction(test_name, backend, navigator, instructions, payload)


# def test_sign_transaction_create(test_name, backend, firmware, navigator):
#     if firmware.device == "nanos":
#         instructions = get_nano_review_instructions(5)
#     elif firmware.device.startswith("nano"):
#         instructions = get_nano_review_instructions(3)
#     else:
#         instructions = get_stax_review_instructions(1)
#     payload = CREATE_ACCOUNT_METHOD_ID.to_bytes(4, byteorder='big')

#     sign_transaction(test_name, backend, navigator, instructions, payload)


# def test_sign_transaction_no_gtw(test_name, backend, firmware, navigator):
#     if firmware.device == "nanos":
#         instructions = get_nano_review_instructions(8)
#     elif firmware.device.startswith("nano"):
#         instructions = get_nano_review_instructions(4)
#     else:
#         instructions = get_stax_review_instructions(1)

#     payload = ""
#     sign_transaction_no_gtw(test_name, backend, navigator, instructions, payload)


# def test_sign_transaction_no_gtw2(test_name, backend, firmware, navigator):
#     if firmware.device == "nanos":
#         instructions = get_nano_review_instructions(9)
#     elif firmware.device.startswith("nano"):
#         instructions = get_nano_review_instructions(5)
#     else:
#         instructions = get_stax_review_instructions_with_warning(1)

#     payload = "1234"
#     sign_transaction_no_gtw(test_name, backend, navigator, instructions, payload)
