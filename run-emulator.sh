#!/usr/bin/env bash
set -euo pipefail

PLATFORM="${1:-basalt}"
VALID_PLATFORMS="aplite basalt chalk diorite emery flint gabbro"

if ! echo "$VALID_PLATFORMS" | grep -qw "$PLATFORM"; then
    echo "Usage: $0 [platform]"
    echo "Platforms: $VALID_PLATFORMS"
    echo "Default: basalt"
    exit 1
fi

REPO_DIR="$(cd "$(dirname "$0")" && pwd)"
PEBBLE_DIR="$REPO_DIR/pebble-app"
COMPANION_DIR="$REPO_DIR/companion-emulator"
VENV_DIR="$COMPANION_DIR/.venv"

cleanup() {
    echo ""
    echo "Shutting down..."
    if [[ -n "${COMPANION_PID:-}" ]] && kill -0 "$COMPANION_PID" 2>/dev/null; then
        kill "$COMPANION_PID" 2>/dev/null
        wait "$COMPANION_PID" 2>/dev/null || true
    fi
    if [[ -n "${PEBBLE_PID:-}" ]] && kill -0 "$PEBBLE_PID" 2>/dev/null; then
        kill "$PEBBLE_PID" 2>/dev/null
        wait "$PEBBLE_PID" 2>/dev/null || true
    fi
    echo "Done."
}
trap cleanup EXIT INT TERM

# --- Build the Pebble app ---
echo "==> Building Pebble app..."
(cd "$PEBBLE_DIR" && pebble build)

# --- Set up companion venv if needed ---
if [[ ! -d "$VENV_DIR" ]]; then
    echo "==> Setting up companion emulator venv..."
    python3 -m venv "$VENV_DIR"
    "$VENV_DIR/bin/pip" install -e "$COMPANION_DIR"
fi

# --- Start Pebble emulator and install app ---
echo "==> Starting Pebble emulator ($PLATFORM) and installing app..."
(cd "$PEBBLE_DIR" && pebble install --emulator "$PLATFORM" --logs) &
PEBBLE_PID=$!

# --- Wait for the emulator to be ready ---
echo "==> Waiting for emulator to start..."
EMULATOR_CONFIG="/tmp/pb-emulator.json"
for i in $(seq 1 30); do
    if [[ -f "$EMULATOR_CONFIG" ]] && python3 -c "
import json, sys
try:
    info = json.load(open('$EMULATOR_CONFIG'))
    p = info.get('$PLATFORM', {})
    pypkjs = p.get('pypkjs', {})
    port = pypkjs.get('port') if isinstance(pypkjs, dict) else pypkjs
    if port:
        sys.exit(0)
except Exception:
    pass
sys.exit(1)
" 2>/dev/null; then
        echo "==> Emulator ready."
        break
    fi
    if ! kill -0 "$PEBBLE_PID" 2>/dev/null; then
        echo "Error: Pebble emulator failed to start."
        exit 1
    fi
    sleep 1
done

# --- Start companion emulator ---
echo "==> Starting companion emulator..."
"$VENV_DIR/bin/pandora-companion" --platform "$PLATFORM" &
COMPANION_PID=$!

echo ""
echo "==> Both emulators running. Press Ctrl+C to stop."
echo ""

wait "$PEBBLE_PID" "$COMPANION_PID" 2>/dev/null || true
