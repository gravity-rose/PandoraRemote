# Companion Emulator

A desktop companion that replaces the Android app during development. It connects
to the Pebble SDK emulator, receives watch button commands, and sends back
simulated Pandora "now playing" metadata.

## Prerequisites

- Python 3.10+
- [Repebble SDK](https://developer.repebble.com) with emulator support

## Installation

```bash
cd companion-emulator
python3 -m venv .venv
source .venv/bin/activate
pip install -e .
```

## Usage

1. Build and start the watch app in the emulator:

```bash
cd ../pebble-app
pebble build
pebble install --emulator basalt
```

2. In another terminal, start the companion:

```bash
cd companion-emulator
source .venv/bin/activate
pandora-companion
```

The companion auto-discovers the emulator's WebSocket endpoint from
`~/.pebble-sdk/pb-emulator.json`. Press buttons on the emulated watch to
send commands — the companion handles them and updates the watch display.

## CLI Options

| Flag | Description |
|------|-------------|
| `--ws-url URL` | WebSocket URL override (auto-discovered by default) |
| `--platform PLATFORM` | Emulator platform for auto-discovery (default: `basalt`) |
| `--qemu-port PORT` | Connect via QEMU transport instead of WebSocket |
| `--log-file PATH` | Write detailed traffic log to file |

## Simulated Behavior

The companion emulates a Pandora-like music service with a hardcoded playlist
of 9 tracks across 3 stations. Button presses on the watch trigger:

| Command | Watch Action | Companion Response |
|---------|-------------|-------------------|
| Thumbs Up | Up (screen 0) | Logs feedback |
| Thumbs Down | Down (screen 0) | Logs feedback, skips to next track |
| Next | Up (screen 1) | Advances to next track |
| Previous | Down (screen 1) | Goes to previous track |
| Play/Pause | Select (screen 1) | Toggles playback state |
| Request Info | App startup | Sends current track metadata |

## Troubleshooting

**"pb-emulator.json not found"** — Start the emulator first with
`pebble install --emulator basalt`.

**"Connection refused"** — The emulator may still be starting up. The companion
retries automatically for up to 30 seconds.

**"No pypkjs port"** — Your emulator config may not include a pypkjs entry.
Try connecting directly with `--ws-url ws://localhost:<port>` or
`--qemu-port <port>`.
