# Copyright 2026 Ledger SAS

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Swap, sell and fund flows of native CELO, run through the Exchange application.

These are the cases Exchange's own suite (app-exchange, test/python/test_celo.py)
runs against a Celo library: the generic ExchangeTestRunner cases, with a valid
transaction signed through the Ethereum client, which speaks the same signing
protocol. Every tampered amount, destination or fee must make the Celo
application refuse with SW_SWAP_CHECKING_FAIL.
"""

from enum import IntEnum

import pytest
from ledger_app_clients.ethereum.client import EthAppClient
from ledger_app_clients.exchange.test_runner import ALL_TESTS_EXCEPT_THORSWAP, ExchangeTestRunner

from . import cal_helper as cal


class Errors(IntEnum):
    SW_SWAP_CHECKING_FAIL = 0xB00A


class CeloTests(ExchangeTestRunner):
    """Native CELO on mainnet."""

    currency_configuration = cal.CELO_CURRENCY_CONFIGURATION

    valid_destination_1 = "0x79D5A290D7ba4b99322d91b577589e8d0BF87072"
    valid_destination_2 = "0x0a101aA5347Bb16F43019BE42ce5830395739e33"
    # Address of the default Speculos seed on cal.CELO_PATH.
    valid_refund = "0x197657b428707c304a5Ee9BA09aF7d13D17BF0D9"
    valid_send_amount_1 = 10000000
    valid_send_amount_2 = 446739662
    valid_fees_1 = 101
    valid_fees_2 = 10078
    fake_refund = "abcdabcd"
    fake_payout = "abcdabcd"

    # Celo mainnet.
    chain_id = 42220

    signature_refusal_error_code = Errors.SW_SWAP_CHECKING_FAIL
    wrong_method_error_code = Errors.SW_SWAP_CHECKING_FAIL
    wrong_destination_error_code = Errors.SW_SWAP_CHECKING_FAIL
    wrong_amount_error_code = Errors.SW_SWAP_CHECKING_FAIL
    wrong_fees_error_code = Errors.SW_SWAP_CHECKING_FAIL

    def perform_final_tx(self, destination, send_amount, fees, memo):
        app_client = EthAppClient(self.backend)
        with app_client.sign(
            bip32_path=cal.CELO_PATH,
            tx_params={
                "nonce": 0,
                "maxFeePerGas": 1,
                "maxPriorityFeePerGas": 3,
                "gas": fees,
                "to": destination,
                "value": send_amount,
                "chainId": self.chain_id,
                # EIP-1559
                "type": 2,
            },
        ):
            pass


class TestsCelo:
    # The snapshots of these cases are named after the test and its parameter, which is
    # how Exchange's suite names them: keep both unchanged.
    @pytest.mark.parametrize("test_to_run", ALL_TESTS_EXCEPT_THORSWAP)
    def test_celo(self, backend, exchange_navigation_helper, test_to_run):
        CeloTests(backend, exchange_navigation_helper).run_test(test_to_run)
