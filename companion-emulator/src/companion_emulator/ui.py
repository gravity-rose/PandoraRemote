import threading
from datetime import datetime

from rich.console import Console
from rich.layout import Layout
from rich.live import Live
from rich.panel import Panel
from rich.table import Table
from rich.text import Text

from .protocol import COMMAND_NAMES
from .state_machine import PandoraStateMachine

MAX_LOG_ENTRIES = 50


class TerminalUI:
    def __init__(self, state_machine: PandoraStateMachine, status: str = "Disconnected"):
        self.state_machine = state_machine
        self.status = status
        self._message_log: list[str] = []
        self._lock = threading.Lock()
        self._live: Live | None = None
        self._console = Console()

    def start(self) -> Live:
        self._live = Live(
            self._build_layout(),
            console=self._console,
            refresh_per_second=4,
            screen=False,
        )
        self._live.start()
        return self._live

    def stop(self) -> None:
        if self._live:
            self._live.stop()
            self._live = None

    def set_status(self, status: str) -> None:
        self.status = status
        self._refresh()

    def log_incoming(self, cmd: int) -> None:
        name = COMMAND_NAMES.get(cmd, f"UNKNOWN({cmd})")
        ts = datetime.now().strftime("%H:%M:%S")
        with self._lock:
            self._message_log.append(f"[cyan]{ts}[/] [bold red]←[/] {name}")
            self._trim_log()
        self._refresh()

    def log_outgoing(self, message: dict) -> None:
        from .protocol import KEY_ARTIST, KEY_PLAY_STATE, KEY_SONG, KEY_STATION

        ts = datetime.now().strftime("%H:%M:%S")
        parts = []
        for key, label in [
            (KEY_STATION, "station"),
            (KEY_ARTIST, "artist"),
            (KEY_SONG, "song"),
            (KEY_PLAY_STATE, "play_state"),
        ]:
            if key in message:
                val = message[key]
                parts.append(f'{label}="{val}"')
        detail = ", ".join(parts)

        with self._lock:
            self._message_log.append(f"[cyan]{ts}[/] [bold green]→[/] {detail}")
            self._trim_log()
        self._refresh()

    def _trim_log(self) -> None:
        if len(self._message_log) > MAX_LOG_ENTRIES:
            self._message_log = self._message_log[-MAX_LOG_ENTRIES:]

    def _refresh(self) -> None:
        if self._live:
            self._live.update(self._build_layout())

    def _build_layout(self) -> Panel:
        layout = Table.grid(padding=(0, 0))
        layout.add_row(self._build_status_panel())
        layout.add_row(self._build_log_panel())
        layout.add_row(self._build_feedback_panel())
        return Panel(layout, title="PANDORA REMOTE — Companion Emulator", border_style="blue")

    def _build_status_panel(self) -> Panel:
        sm = self.state_machine
        track = sm.current_track
        play_icon = "▶" if sm.is_playing else "⏸"
        state_text = "PLAYING" if sm.is_playing else "PAUSED"

        table = Table.grid(padding=(0, 1))
        table.add_column(style="bold", width=10)
        table.add_column()
        table.add_row("Status:", self.status)
        table.add_row("State:", f"{play_icon} {state_text}")
        table.add_row("Station:", track.station)
        table.add_row("Artist:", track.artist)
        table.add_row("Song:", track.song)
        table.add_row("Track:", f"{sm.current_index + 1} / {len(sm.playlist)}")

        return Panel(table, title="Now Playing", border_style="green")

    def _build_log_panel(self) -> Panel:
        with self._lock:
            if self._message_log:
                text = Text.from_markup("\n".join(reversed(self._message_log[-15:])))
            else:
                text = Text("No messages yet...", style="dim")
        return Panel(text, title="Message Log", border_style="yellow", height=18)

    def _build_feedback_panel(self) -> Panel:
        entries = self.state_machine.feedback_log[-5:]
        if entries:
            lines = []
            for entry in reversed(entries):
                ts = entry.timestamp.strftime("%H:%M:%S")
                action = entry.action.upper().replace("_", " ")
                lines.append(
                    f"[cyan]{ts}[/] {action}: "
                    f'"{entry.track.song}" by {entry.track.artist}'
                )
            text = Text.from_markup("\n".join(lines))
        else:
            text = Text("No feedback yet...", style="dim")
        return Panel(text, title="Feedback Log", border_style="magenta", height=8)
