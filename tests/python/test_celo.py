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
    assert (
        response.data.hex()
        == "4104f3c5b892381bdc277026f0675634fe6b8f339a58e689faede9be2ba5701fd215599bae802930025fc0bd270136d9081266741d8bf562e5ca70da9ff95a1942bc2846343935466332624331383238314533353244626632383039393432366330343237363236374463"
    )


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
    assert (
        response.data.hex()
        == "1c76536c41e8830a322517f921333237a616726ac151807558aeaa10f9efb8d1937299f8ad544f412264fb6467732bedc5215231f3dfe44444e2bc3a9597ee0eae"
    )
