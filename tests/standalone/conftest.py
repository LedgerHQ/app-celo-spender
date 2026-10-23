import atexit
import os
import re
import threading
import time
import uuid
from pathlib import Path
from typing import List, Optional

import pytest
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


# Parallel runs with pytest-xdist: every worker drives its own Speculos instance.
# Adapted from app-tezos, tests/standalone/conftest.py (Apache-2.0, Copyright 2024
# Trilitech and Functori).


def _xdist_worker_index() -> Optional[int]:
    """Index of this pytest-xdist worker (gw3 -> 3), or None outside a worker."""
    match = re.fullmatch(r"gw(\d+)", os.environ.get("PYTEST_XDIST_WORKER", ""))
    return int(match.group(1)) if match else None


# Upper bound on the workers `-n auto` starts (override: SPECULOS_MAX_WORKERS).
DEFAULT_MAX_SPECULOS_WORKERS = 16

# Runs of this suite that share the repository directory (one container per target) record
# themselves here, so each can split the cores between the runs that are alive right now.
PARALLEL_RUNS_DIR = Path(__file__).parent / ".parallel-runs"
_RUN_HEARTBEAT_SECONDS = 5.0
_RUN_STALE_AFTER_SECONDS = 15.0
# Time to let simultaneously launched runs register before counting them.
_RUN_REGISTRATION_GRACE_SECONDS = 3.0

_run_state: dict = {"file": None}


def _unregister_run() -> None:
    """Remove this run's file from the shared directory."""
    run_file, _run_state["file"] = _run_state["file"], None
    if run_file is not None:
        run_file.unlink(missing_ok=True)


def _register_run(device: str) -> int:
    """Record this run in the shared directory and return how many runs are alive, itself included.

    A run is alive while its file is touched; a crashed run goes stale and stops being counted.
    """
    PARALLEL_RUNS_DIR.mkdir(exist_ok=True)
    run_file = PARALLEL_RUNS_DIR / f"{device}-{uuid.uuid4().hex}.run"
    run_file.touch()
    _run_state["file"] = run_file

    def heartbeat() -> None:
        while _run_state["file"] is run_file:
            time.sleep(_RUN_HEARTBEAT_SECONDS)
            try:
                run_file.touch()
            except OSError:
                return

    threading.Thread(target=heartbeat, daemon=True).start()
    atexit.register(_unregister_run)

    time.sleep(_RUN_REGISTRATION_GRACE_SECONDS)
    alive = 0
    for other in PARALLEL_RUNS_DIR.glob("*.run"):
        try:
            if time.time() - other.stat().st_mtime < _RUN_STALE_AFTER_SECONDS:
                alive += 1
        except FileNotFoundError:
            continue
    return max(1, alive)


@pytest.hookimpl(optionalhook=True)
def pytest_xdist_auto_num_workers(config) -> Optional[int]:
    """Worker count for `-n auto`: the usable cores divided by the runs alive now, capped.

    Every per-target container sees all the cores of the host, so `-n auto` in each of five
    containers would start five times too many Speculos instances, and the UI waits become
    timing-sensitive. The runs sharing this directory count each other (see _register_run),
    so a lone run gets all the cores and five concurrent ones get a fifth each. The usable
    cores are the CPU affinity mask, which follows a container's --cpuset-cpus
    (os.cpu_count() would count the host's).

    Runs launched more than a few seconds apart do not see each other at start-up: the
    earlier ones take more than their share. SPECULOS_PARALLEL_TARGETS fixes the divisor
    instead of counting, SPECULOS_MAX_WORKERS caps the result, and an explicit
    PYTEST_XDIST_AUTO_NUM_WORKERS overrides everything (xdist applies it).
    """
    if "PYTEST_XDIST_AUTO_NUM_WORKERS" in os.environ:
        return None
    cap = int(os.environ.get("SPECULOS_MAX_WORKERS", DEFAULT_MAX_SPECULOS_WORKERS))
    cores = len(os.sched_getaffinity(0)) if hasattr(os, "sched_getaffinity") else os.cpu_count() or 1
    if "SPECULOS_PARALLEL_TARGETS" in os.environ:
        runs = int(os.environ["SPECULOS_PARALLEL_TARGETS"])
    else:
        runs = _register_run(str(config.getoption("device", default="run")))
    return max(1, min(cap, cores // max(1, runs)))


def pytest_addoption(parser):
    """Register the Speculos port options used under pytest-xdist.

    Each worker gets its own ports: API port base + index * stride, APDU port one above.
    """
    parser.addoption(
        "--speculos-base-port",
        type=int,
        default=int(os.environ.get("SPECULOS_BASE_PORT", "5000")),
        help="First Speculos API port under pytest-xdist (env: SPECULOS_BASE_PORT)",
    )
    parser.addoption(
        "--speculos-port-stride",
        type=int,
        default=int(os.environ.get("SPECULOS_PORT_STRIDE", "10")),
        help="Ports reserved per pytest-xdist worker, at least 2 (env: SPECULOS_PORT_STRIDE)",
    )


@pytest.fixture(scope=configuration.OPTIONAL.BACKEND_SCOPE)
def additional_speculos_arguments(request) -> List[str]:
    """Distinct Speculos ports per xdist worker; ragger's own choice outside xdist.

    Ragger otherwise picks the first free port, and workers starting together can all see
    the same one as free.
    """
    index = _xdist_worker_index()
    if index is None:
        return []
    stride = request.config.getoption("--speculos-port-stride")
    if stride < 2:
        raise pytest.UsageError("--speculos-port-stride must be at least 2 (API + APDU port)")
    api_port = request.config.getoption("--speculos-base-port") + index * stride
    return ["--api-port", str(api_port), "--apdu-port", str(api_port + 1)]


def pytest_sessionfinish(session):
    """Leave the shared run directory as soon as the session is over, so other runs stop counting this one."""
    _ = session
    _unregister_run()
