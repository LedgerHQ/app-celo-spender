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

"""Coin configurations, as the CAL would provide them to the Exchange application."""

from ledger_app_clients.exchange.cal_helper import CurrencyConfiguration
from ragger.bip import pack_derivation_path
from ragger.utils import create_currency_config

CELO_PATH = "m/44'/52752'/0'/0/0"
CELO_PACKED_DERIVATION_PATH = pack_derivation_path(CELO_PATH)

# Native CELO. The sub-coin configuration is what the Celo application receives
# from Exchange: the ticker and decimals its swap check compares the signed
# transaction against.
CELO_CURRENCY_CONFIGURATION = CurrencyConfiguration(
    ticker="CELO",
    conf=create_currency_config("CELO", "Celo", sub_coin_config=("CELO", 18)),
    packed_derivation_path=CELO_PACKED_DERIVATION_PATH,
)
