import threading
from datetime import datetime

import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, GLib, Pango

from .protocol import COMMAND_NAMES, KEY_ARTIST, KEY_PLAY_STATE, KEY_SONG, KEY_STATION
from .state_machine import PandoraStateMachine

MAX_LOG_ENTRIES = 200


class TerminalUI:
    def __init__(self, state_machine: PandoraStateMachine, status: str = "Disconnected"):
        self.state_machine = state_machine
        self.status = status
        self._window = None
        self._gtk_thread = None

    def start(self):
        self._gtk_thread = threading.Thread(target=self._run_gtk, daemon=True)
        self._gtk_thread.start()

    def stop(self):
        if self._window:
            GLib.idle_add(Gtk.main_quit)

    def set_status(self, status: str):
        self.status = status
        GLib.idle_add(self._update_status_panel)

    def log_incoming(self, cmd: int):
        name = COMMAND_NAMES.get(cmd, f"UNKNOWN({cmd})")
        ts = datetime.now().strftime("%H:%M:%S")
        GLib.idle_add(self._append_log, f"[{ts}] ← {name}")

    def log_outgoing(self, message: dict):
        ts = datetime.now().strftime("%H:%M:%S")
        parts = []
        for key, label in [
            (KEY_STATION, "station"),
            (KEY_ARTIST, "artist"),
            (KEY_SONG, "song"),
            (KEY_PLAY_STATE, "play_state"),
        ]:
            if key in message:
                parts.append(f'{label}="{message[key]}"')
        detail = ", ".join(parts)
        GLib.idle_add(self._append_log, f"[{ts}] → {detail}")
        GLib.idle_add(self._update_status_panel)

    def _run_gtk(self):
        self._window = Gtk.Window(title="PandoraRemote — Companion Emulator")
        self._window.set_default_size(420, 520)
        self._window.connect("destroy", Gtk.main_quit)

        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
        vbox.set_margin_top(8)
        vbox.set_margin_bottom(8)
        vbox.set_margin_start(8)
        vbox.set_margin_end(8)
        self._window.add(vbox)

        # --- Now Playing ---
        status_frame = Gtk.Frame(label="Now Playing")
        status_grid = Gtk.Grid(column_spacing=12, row_spacing=4)
        status_grid.set_margin_top(8)
        status_grid.set_margin_bottom(8)
        status_grid.set_margin_start(12)
        status_grid.set_margin_end(12)

        labels = ["Status:", "State:", "Station:", "Artist:", "Song:", "Track:"]
        self._status_values = {}
        for i, label_text in enumerate(labels):
            label = Gtk.Label(label=label_text, xalign=0)
            label.set_markup(f"<b>{label_text}</b>")
            status_grid.attach(label, 0, i, 1, 1)
            value = Gtk.Label(label="", xalign=0)
            value.set_selectable(True)
            value.set_line_wrap(True)
            value.set_hexpand(True)
            status_grid.attach(value, 1, i, 1, 1)
            key = label_text.rstrip(":").lower()
            self._status_values[key] = value

        status_frame.add(status_grid)
        vbox.pack_start(status_frame, False, False, 0)

        # --- Message Log ---
        log_frame = Gtk.Frame(label="Message Log")
        self._log_buffer = Gtk.TextBuffer()
        log_view = Gtk.TextView(buffer=self._log_buffer)
        log_view.set_editable(False)
        log_view.set_cursor_visible(False)
        log_view.set_wrap_mode(Gtk.WrapMode.WORD_CHAR)
        log_view.override_font(Pango.FontDescription("monospace 9"))
        self._log_view = log_view

        log_scroll = Gtk.ScrolledWindow()
        log_scroll.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        log_scroll.set_min_content_height(180)
        log_scroll.add(log_view)
        log_frame.add(log_scroll)
        vbox.pack_start(log_frame, True, True, 0)

        # --- Feedback Log ---
        feedback_frame = Gtk.Frame(label="Feedback Log")
        self._feedback_buffer = Gtk.TextBuffer()
        feedback_view = Gtk.TextView(buffer=self._feedback_buffer)
        feedback_view.set_editable(False)
        feedback_view.set_cursor_visible(False)
        feedback_view.set_wrap_mode(Gtk.WrapMode.WORD_CHAR)
        feedback_view.override_font(Pango.FontDescription("monospace 9"))

        feedback_scroll = Gtk.ScrolledWindow()
        feedback_scroll.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        feedback_scroll.set_min_content_height(80)
        feedback_scroll.add(feedback_view)
        feedback_frame.add(feedback_scroll)
        vbox.pack_start(feedback_frame, False, False, 0)

        self._update_status_panel()
        self._window.show_all()
        Gtk.main()

    def _update_status_panel(self):
        sm = self.state_machine
        track = sm.current_track
        play_icon = "▶" if sm.is_playing else "⏸"
        state_text = "PLAYING" if sm.is_playing else "PAUSED"

        self._status_values["status"].set_text(self.status)
        self._status_values["state"].set_text(f"{play_icon} {state_text}")
        self._status_values["station"].set_text(track.station)
        self._status_values["artist"].set_text(track.artist)
        self._status_values["song"].set_text(track.song)
        self._status_values["track"].set_text(f"{sm.current_index + 1} / {len(sm.playlist)}")

        entries = sm.feedback_log[-5:]
        if entries:
            lines = []
            for entry in reversed(entries):
                ts = entry.timestamp.strftime("%H:%M:%S")
                action = entry.action.upper().replace("_", " ")
                lines.append(f"[{ts}] {action}: \"{entry.track.song}\" by {entry.track.artist}")
            self._feedback_buffer.set_text("\n".join(lines))
        else:
            self._feedback_buffer.set_text("No feedback yet...")

    def _append_log(self, text: str):
        end_iter = self._log_buffer.get_end_iter()
        if self._log_buffer.get_char_count() > 0:
            self._log_buffer.insert(end_iter, "\n")
            end_iter = self._log_buffer.get_end_iter()
        self._log_buffer.insert(end_iter, text)

        line_count = self._log_buffer.get_line_count()
        if line_count > MAX_LOG_ENTRIES:
            start = self._log_buffer.get_start_iter()
            trim_end = self._log_buffer.get_iter_at_line(line_count - MAX_LOG_ENTRIES)
            self._log_buffer.delete(start, trim_end)

        # Auto-scroll to bottom
        end_mark = self._log_buffer.create_mark(None, self._log_buffer.get_end_iter(), False)
        self._log_view.scroll_to_mark(end_mark, 0, False, 0, 0)
        self._log_buffer.delete_mark(end_mark)
