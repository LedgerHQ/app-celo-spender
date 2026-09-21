#!/usr/bin/env bash
# Build and run the host unit tests (tests/unit, cmocka) inside the Ledger app-builder image,
# the same way .github/workflows/ci-workflow.yml does.
#
# Usage:
#   ./scripts/unit-tests-docker.sh                 # build + run every suite
#   ./scripts/unit-tests-docker.sh --clean         # drop the build dir first
#   ./scripts/unit-tests-docker.sh -R test_uint256 # extra args go to ctest
#
# Override the image:
#   UNIT_TESTS_IMAGE=ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder-lite:latest \
#   ./scripts/unit-tests-docker.sh

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE="${UNIT_TESTS_IMAGE:-ghcr.io/ledgerhq/ledger-app-builder/ledger-app-dev-tools:latest}"
TEST_DIR="${UNIT_TESTS_DIR:-tests/unit}"

CLEAN=0
if [[ "${1:-}" == "--clean" ]]; then
    CLEAN=1
    shift
fi

docker run --rm \
    --user "$(id -u):$(id -g)" \
    -v "${ROOT}:/app" \
    -w "/app/${TEST_DIR}" \
    -e "CLEAN=${CLEAN}" \
    -e "CTEST_OUTPUT_ON_FAILURE=1" \
    "${IMAGE}" \
    bash -c '
set -euo pipefail
[[ "${CLEAN}" == "1" ]] && rm -rf build
cmake -Bbuild -H.
make -C build
ctest --test-dir build "$@"
' -- "$@"
