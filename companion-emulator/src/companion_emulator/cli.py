import argparse
import logging
import signal
import sys
import threading

from libpebble2.services.appmessage import AppMessageService

from .connection import connect_qemu, connect_websocket, discover_ws_url
from .protocol import APP_UUID, COMMAND_NAMES, parse_command
from .state_machine import PandoraStateMachine
from .ui import TerminalUI

logger = logging.getLogger("companion_emulator")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="pandora-companion",
        description="Companion emulator for PandoraRemote development.",
    )
    parser.add_argument(
        "--ws-url",
        help="WebSocket URL (e.g., ws://localhost:9000). Auto-discovered if not set.",
    )
    parser.add_argument(
        "--platform",
        default="basalt",
        help="Emulator platform for auto-discovery (default: basalt).",
    )
    parser.add_argument(
        "--qemu-port",
        type=int,
        help="Connect via QEMU transport on this port instead of WebSocket.",
    )
    parser.add_argument(
        "--log-file",
        help="Write detailed traffic log to this file.",
    )
    return parser


def main() -> None:
    args = build_parser().parse_args()

    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(name)s %(levelname)s %(message)s",
        handlers=[logging.StreamHandler(sys.stderr)],
    )

    if args.log_file:
        file_handler = logging.FileHandler(args.log_file)
        file_handler.setLevel(logging.DEBUG)
        file_handler.setFormatter(
            logging.Formatter("%(asctime)s %(name)s %(levelname)s %(message)s")
        )
        logging.getLogger().addHandler(file_handler)
        logging.getLogger().setLevel(logging.DEBUG)

    state_machine = PandoraStateMachine()
    ui = TerminalUI(state_machine, status="Connecting...")

    ui.start()
    try:
        if args.qemu_port:
            pebble = connect_qemu(args.qemu_port)
            ui.set_status(f"Connected via QEMU port {args.qemu_port}")
        else:
            ws_url = args.ws_url or discover_ws_url(args.platform)
            pebble = connect_websocket(ws_url)
            ui.set_status(f"Connected to {ws_url}")
    except (ConnectionError, FileNotFoundError, KeyError, ValueError) as e:
        ui.stop()
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

    appmsg = AppMessageService(pebble)

    def on_message(transaction_id, uuid, data):
        cmd = parse_command(data)
        if cmd is None:
            logger.debug("Received message without KEY_COMMAND: %s", data)
            return

        name = COMMAND_NAMES.get(cmd, f"UNKNOWN({cmd})")
        logger.info("Received command: %s (%d)", name, cmd)
        ui.log_incoming(cmd)

        response = state_machine.handle_command(cmd)
        if response is not None:
            appmsg.send_message(APP_UUID, response)
            ui.log_outgoing(response)
            logger.info("Sent response for %s", name)

    appmsg.register_handler("appmessage", on_message)

    initial = state_machine.get_metadata_message()
    appmsg.send_message(APP_UUID, initial)
    ui.log_outgoing(initial)
    logger.info("Sent initial metadata")

    stop_event = threading.Event()

    def handle_signal(signum, frame):
        stop_event.set()

    signal.signal(signal.SIGINT, handle_signal)
    signal.signal(signal.SIGTERM, handle_signal)

    try:
        stop_event.wait()
    finally:
        logger.info("Shutting down...")
        appmsg.shutdown()
        ui.stop()
        print("\nGoodbye!")
