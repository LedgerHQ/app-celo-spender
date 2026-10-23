# Swap functional tests

These tests exercise the Celo application as it is used by the
[Exchange application](https://ledgerhq.github.io/app-exchange/) through the
`os_lib_call` mechanism, i.e. the swap feature. That mode takes a different
launch path in the application and needs a different Speculos setup, which is
why it lives beside `tests/standalone/` rather than inside it.

Speculos runs the Exchange application, with the Celo application and the
Ethereum application - which serves as the payout currency - sideloaded as
libraries.

```text
swap/
├── cal_helper.py                     # Coin configurations, as the CAL would provide them
├── conftest.py                       # Ragger configuration and fixtures
├── test_swap_celo.py                 # The test cases
├── helper_tool_clone_dependencies.py # Clones Exchange and Ethereum (run on the host)
├── helper_tool_build_dependencies.py # Builds them (run inside the Ledger docker image)
├── snapshots/                        # Ragger UI snapshots, of the Exchange screens
└── requirements.txt                  # Python dependencies
```

## Setup

Build the Celo application for the device you want to test, in the Ledger
docker environment, as usual.

Then clone the Exchange and Ethereum applications, on the host:

```sh
pip install -U GitPython
python tests/swap/helper_tool_clone_dependencies.py
```

And build them, from the `tests/swap` directory, inside the docker image:

```sh
docker run --user "$(id -u)":"$(id -g)" --rm -ti -v "$(realpath .):/app" \
    "ghcr.io/ledgerhq/ledger-app-builder/ledger-app-dev-tools:latest" \
    python3 helper_tool_build_dependencies.py
```

This drops the binaries where `conftest.py` expects them:
`.test_dependencies/main/app-exchange/` and
`.test_dependencies/libraries/app-ethereum/`.

## Running

```sh
pip install -r tests/swap/requirements.txt
pytest tests/swap --device flex
```

Add `--golden_run` to regenerate the snapshots, and `-k <name>` to run a single
case. In CI the same is done by the `tests_swap` job, which builds the
dependencies declared under `[pytest.swap.dependencies]` in `ledger_app.toml`.

Unlike `tests/standalone/`, this suite is not run in parallel: its Speculos
instance is shared by the cases of a class, and the cases are kept in order for
that reason (see `pytest_collection_modifyitems` in `conftest.py`).

## What is covered

`test_celo` runs the generic cases of Exchange's `ExchangeTestRunner` for native
CELO on Celo mainnet, THORSwap excepted. They are the cases Exchange's own suite
(`test/python/test_celo.py` in app-exchange) runs against a Celo library, and
the snapshots are named after the test and its parameter exactly as there.

| Case | Expectation |
| --- | --- |
| `swap_valid_*`, `sell_valid_*`, `fund_valid_*` | A transaction that matches what Exchange approved is signed. |
| `swap_wrong_amount`, `sell_wrong_amount`, `fund_wrong_amount` | A different amount is refused with `SW_SWAP_CHECKING_FAIL` (0xB00A). |
| `swap_wrong_destination`, `sell_wrong_destination`, `fund_wrong_destination` | A different recipient is refused with `SW_SWAP_CHECKING_FAIL`. |
| `swap_wrong_fees`, `sell_wrong_fees`, `fund_wrong_fees` | Fees above the approved ones are refused with `SW_SWAP_CHECKING_FAIL`. |
| `swap_wrong_refund` | A refund address the Celo application cannot parse is refused with `INVALID_ADDRESS`, before any review. |
| `swap_wrong_memo`, `sell_wrong_memo`, `fund_wrong_memo` | Skipped: Celo has no memo, so there is no second value to tamper with. |
| `swap_ui_only` | The Exchange review screens of a swap, without starting the Celo application. |

The final transaction is signed with the Ethereum client, which speaks the same
signing protocol as the Celo application: an EIP-1559 transaction on chain ID
42220 with a plain native transfer. Swaps of tokens, and anything with calldata,
are not covered here.
