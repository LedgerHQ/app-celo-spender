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

"""Conftest for the swap tests.

In this suite the Celo application is not the application under test but a
library, called by the Exchange application through os_lib_call(). Speculos is
therefore started on Exchange, with the Celo application - and Ethereum, which
serves as the payout currency - sideloaded.

No custom seed is set: the swap addresses in test_swap_celo.py are those of the
default Speculos seed.
"""

from pathlib import Path

import pytest
from ledger_app_clients.exchange.navigation_helper import ExchangeNavigationHelper
from ragger.conftest import configuration

###########################
### CONFIGURATION START ###
###########################

configuration.OPTIONAL.BACKEND_SCOPE = "class"
# Exchange, the application Speculos starts. See tests/swap/README.md for how
# to populate these directories.
configuration.OPTIONAL.MAIN_APP_DIR = "tests/swap/.test_dependencies/main"

# Ethereum, sideloaded so Exchange can check the payout address: its variant, and its name
# on the device. The Ledger VS Code extension reads these two declarations to build the
# app with "Prepare Test Dependencies", as <directory>/<variant>_<device>.elf. They have
# to stay plain literals for it to read them without running Python.
SIDELOADED_APPS = {"ethereum": "Ethereum"}
SIDELOADED_APPS_DIR = "tests/swap/.test_dependencies/libraries/"
configuration.OPTIONAL.SIDELOADED_APPS_DIR = SIDELOADED_APPS_DIR

#########################
### CONFIGURATION END ###
#########################

# Pull all features from the base ragger conftest using the overridden configuration
pytest_plugins = ("ragger.conftest.base_conftest",)


def pytest_configure(config):
    """Tell Ragger how to find the sideloaded apps, according to what has been built.

    Given a list of sideloaded apps, Ragger only looks for flat <variant>_<device>.elf
    files, which is what the extension produces, and ignores the app-<name>/build
    directories that the helper scripts and the CI produce. The list is therefore only
    handed over when such files exist; otherwise Ragger scans the directory itself.
    """
    libraries = Path(__file__).resolve().parents[2] / SIDELOADED_APPS_DIR
    if any(binary for variant in SIDELOADED_APPS for binary in libraries.glob(f"{variant}_*.elf")):
        configuration.OPTIONAL.SIDELOADED_APPS = SIDELOADED_APPS


@pytest.fixture(scope="session", autouse=True)
def swap_dependencies():
    """Stop with the setup steps when Exchange or Ethereum have not been built.

    Without them Speculos has no application to start, and ragger reports it as a bare
    FileNotFoundError from deep inside its backend fixture.
    """
    dependencies_dir = Path(__file__).parent / ".test_dependencies"
    how_to_build = {
        "main": "Exchange: run helper_tool_clone_dependencies.py exchange, then helper_tool_build_dependencies.py exchange",
        "libraries": "Ethereum: run 'Prepare Test Dependencies' in the Ledger VS Code extension, or the same two scripts with ethereum",
    }
    missing = [
        f"{dependencies_dir / directory} ({hint})"
        for directory, hint in how_to_build.items()
        if not ((dependencies_dir / directory).is_dir() and any((dependencies_dir / directory).iterdir()))
    ]
    if missing:
        pytest.exit(
            "Nothing built in " + "; ".join(missing) + ". See 'Setup' in tests/swap/README.md.",
            returncode=pytest.ExitCode.USAGE_ERROR,
        )


@pytest.fixture(scope="session")
def snapshots_path():
    """Provide the default path for screenshots, used by ExchangeNavigationHelper."""
    return Path(__file__).parent.resolve()


@pytest.fixture(scope="function")
def exchange_navigation_helper(backend, navigator, snapshots_path, test_name):
    """Drive the Exchange screens and compare them against the snapshots."""
    return ExchangeNavigationHelper(backend=backend, navigator=navigator, snapshots_path=snapshots_path, test_name=test_name)


def pytest_collection_modifyitems(config, items):
    """Keep the parametrized tests in node id order.

    Pytest reorders tests using parametrize by alphabetical order of the
    parameter, which breaks the backend scope optimisation.
    """
    items[:] = sorted(items, key=lambda item: item.nodeid)
