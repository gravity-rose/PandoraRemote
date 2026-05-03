import json
import logging
import tempfile
import time
from pathlib import Path

from libpebble2.communication import PebbleConnection
from libpebble2.communication.transports.websocket import WebsocketTransport
from libpebble2.communication.transports.qemu import QemuTransport

logger = logging.getLogger(__name__)

EMULATOR_CONFIG_PATH = Path(tempfile.gettempdir()) / "pb-emulator.json"
MAX_RETRIES = 10
RETRY_DELAY = 3


def discover_ws_url(platform: str = "basalt") -> str:
    if not EMULATOR_CONFIG_PATH.exists():
        raise FileNotFoundError(
            f"{EMULATOR_CONFIG_PATH} not found. "
            f"Start the emulator first: pebble install --emulator {platform}"
        )

    with open(EMULATOR_CONFIG_PATH) as f:
        config = json.load(f)

    if platform not in config:
        available = ", ".join(config.keys()) or "(none)"
        raise KeyError(
            f"Platform '{platform}' not found in emulator config. "
            f"Available: {available}"
        )

    entry = config[platform]
    # Handle versioned config: basalt -> "4.9.169" -> pypkjs
    if "pypkjs" not in entry and "qemu" not in entry:
        for val in entry.values():
            if isinstance(val, dict) and ("pypkjs" in val or "qemu" in val):
                entry = val
                break
    pypkjs_info = entry.get("pypkjs", {})
    pypkjs_port = pypkjs_info.get("port") if isinstance(pypkjs_info, dict) else pypkjs_info
    if pypkjs_port is None:
        qemu_info = entry.get("qemu", {})
        qemu_port = qemu_info.get("serial") if isinstance(qemu_info, dict) else qemu_info
        if qemu_port:
            raise ValueError(
                f"No pypkjs port for '{platform}', but QEMU serial port "
                f"{qemu_port} is available. Use --qemu-port {qemu_port}"
            )
        raise ValueError(f"No pypkjs port found for platform '{platform}'")

    return f"ws://localhost:{pypkjs_port}"


def connect_websocket(ws_url: str) -> PebbleConnection:
    logger.info("Connecting to %s", ws_url)

    for attempt in range(1, MAX_RETRIES + 1):
        try:
            transport = WebsocketTransport(ws_url)
            pebble = PebbleConnection(transport)
            pebble.connect()
            pebble.run_async()
            logger.info("Connected to emulator")
            return pebble
        except ConnectionError:
            if attempt < MAX_RETRIES:
                logger.warning(
                    "Connection failed (attempt %d/%d), retrying in %ds...",
                    attempt,
                    MAX_RETRIES,
                    RETRY_DELAY,
                )
                time.sleep(RETRY_DELAY)
            else:
                raise ConnectionError(
                    f"Could not connect to {ws_url} after {MAX_RETRIES} attempts. "
                    "Is the emulator running?"
                )


def connect_qemu(port: int) -> PebbleConnection:
    logger.info("Connecting to QEMU on port %d", port)
    transport = QemuTransport("localhost", port)
    pebble = PebbleConnection(transport)
    pebble.connect()
    pebble.run_async()
    logger.info("Connected to emulator via QEMU")
    return pebble
