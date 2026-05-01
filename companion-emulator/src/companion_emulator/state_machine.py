from dataclasses import dataclass
from datetime import datetime

from .playlist import DEFAULT_PLAYLIST, Track
from .protocol import (
    CMD_NEXT,
    CMD_PLAY_PAUSE,
    CMD_PREVIOUS,
    CMD_REQUEST_INFO,
    CMD_THUMBS_DOWN,
    CMD_THUMBS_UP,
    build_metadata_message,
    build_play_state_message,
)


@dataclass
class FeedbackEntry:
    timestamp: datetime
    track: Track
    action: str


class PandoraStateMachine:
    def __init__(self, playlist: list[Track] | None = None):
        self.playlist = playlist or list(DEFAULT_PLAYLIST)
        self.current_index: int = 0
        self.is_playing: bool = True
        self.feedback_log: list[FeedbackEntry] = []

    @property
    def current_track(self) -> Track:
        return self.playlist[self.current_index]

    def handle_command(self, cmd: int) -> dict | None:
        if cmd == CMD_REQUEST_INFO:
            return self.get_metadata_message()
        elif cmd == CMD_PLAY_PAUSE:
            return self._toggle_play_pause()
        elif cmd == CMD_NEXT:
            return self._advance(1)
        elif cmd == CMD_PREVIOUS:
            return self._advance(-1)
        elif cmd == CMD_THUMBS_UP:
            self._log_feedback("thumbs_up")
            return None
        elif cmd == CMD_THUMBS_DOWN:
            self._log_feedback("thumbs_down")
            return self._advance(1)
        return None

    def _advance(self, delta: int = 1) -> dict:
        self.current_index = (self.current_index + delta) % len(self.playlist)
        return self.get_metadata_message()

    def _toggle_play_pause(self) -> dict:
        self.is_playing = not self.is_playing
        return build_play_state_message(self.is_playing)

    def _log_feedback(self, action: str) -> None:
        self.feedback_log.append(
            FeedbackEntry(
                timestamp=datetime.now(),
                track=self.current_track,
                action=action,
            )
        )

    def get_metadata_message(self) -> dict:
        track = self.current_track
        return build_metadata_message(
            track.station, track.artist, track.song, self.is_playing
        )
