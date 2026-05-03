import uuid as _uuid

from libpebble2.services.appmessage import CString, Int32

APP_UUID = _uuid.UUID("87b85f04-54ce-4b5e-8941-14a46c9d3855")

KEY_COMMAND = 0
KEY_STATION = 1
KEY_ARTIST = 2
KEY_SONG = 3
KEY_PLAY_STATE = 4

CMD_THUMBS_UP = 1
CMD_THUMBS_DOWN = 2
CMD_NEXT = 3
CMD_PREVIOUS = 4
CMD_PLAY_PAUSE = 5
CMD_REQUEST_INFO = 6

COMMAND_NAMES = {
    CMD_THUMBS_UP: "THUMBS_UP",
    CMD_THUMBS_DOWN: "THUMBS_DOWN",
    CMD_NEXT: "NEXT",
    CMD_PREVIOUS: "PREVIOUS",
    CMD_PLAY_PAUSE: "PLAY_PAUSE",
    CMD_REQUEST_INFO: "REQUEST_INFO",
}


def build_metadata_message(
    station: str, artist: str, song: str, is_playing: bool
) -> dict:
    return {
        KEY_STATION: CString(station),
        KEY_ARTIST: CString(artist),
        KEY_SONG: CString(song),
        KEY_PLAY_STATE: Int32(1 if is_playing else 0),
    }


def build_play_state_message(is_playing: bool) -> dict:
    return {
        KEY_PLAY_STATE: Int32(1 if is_playing else 0),
    }


def parse_command(data: dict) -> int | None:
    value = data.get(KEY_COMMAND)
    if value is None:
        return None
    return int(value)
