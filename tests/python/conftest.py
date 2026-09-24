from typing import List, Optional

from ragger.conftest import configuration

from .apps.celo import StatusCode

###########################
### CONFIGURATION START ###
###########################

# You can configure optional parameters by overriding the value of ragger.configuration.OPTIONAL_CONFIGURATION
# Please refer to ragger/conftest/configuration.py for their descriptions and accepted values

#########################
### CONFIGURATION END ###
#########################

# Pull all features from the base ragger conftest using the overridden configuration
pytest_plugins = ("ragger.conftest.base_conftest", )


def _format_status(value: int) -> str:
    try:
        return f"0x{value:04X} ({StatusCode(value).name})"
    except ValueError:
        return f"0x{value:04X}"


def pytest_assertrepr_compare(op: str, left: object, right: object) -> Optional[List[str]]:
    """Report status word comparisons in hex, the notation the app and the APDU docs use."""
    if op != "==" or not isinstance(left, int) or not isinstance(right, int):
        return None
    if not isinstance(left, StatusCode) and not isinstance(right, StatusCode):
        return None
    return [f"{_format_status(left)} == {_format_status(right)}"]
