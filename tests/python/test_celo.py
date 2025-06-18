from pathlib import Path

from .apps.celo import CeloClient, StatusCode
from .apps.celo_utils import CELO_PACKED_DERIVATION_PATH
from .utils import (
    get_async_response,
    get_nano_review_instructions,
    get_stax_review_instructions,
    get_stax_review_instructions_with_warning,
)
from ragger.navigator import NavInsID, NavIns


import pytest

TESTS_ROOT_DIR = Path(__file__).parent


@pytest.mark.parametrize("show", [False, True])
@pytest.mark.parametrize("chaincode", [False, True])
@pytest.mark.active_test_scope
def test_celo_derive_address(
    test_name, backend, firmware, show, chaincode, scenario_navigator
):
    celo = CeloClient(backend)

    with celo.derive_address_async(CELO_PACKED_DERIVATION_PATH, show, chaincode):
        if show:
            scenario_navigator.address_review_approve(TESTS_ROOT_DIR, test_name)

    response: bytes = get_async_response(backend)

    assert response.status == StatusCode.STATUS_OK


# @pytest.mark.active_test_scope
def test_celo_get_version(backend, firmware):
    celo = CeloClient(backend)
    response = celo.get_version()

    assert response.status == StatusCode.STATUS_OK


# @pytest.mark.active_test_scope
def test_sign_data(test_name, backend, firmware, scenario_navigator):
    celo = CeloClient(backend)

    with celo.sign_data_async(CELO_PACKED_DERIVATION_PATH, "1234567890"):
        scenario_navigator.review_approve(TESTS_ROOT_DIR, test_name)

    response: bytes = get_async_response(backend)

    assert response.status == StatusCode.STATUS_OK
    assert len(response.data) == 65
