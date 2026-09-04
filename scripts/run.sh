#!/usr/bin/env bash
set -e

# Default parameter values
BRIDGE_PATH="${1:-build/MuehleBridge}"
HOST="${2:-0.0.0.0}"
PORT="${3:-8787}"
TIMEOUT="${4:-10}"

# Verify executable exists
if [ ! -f "$BRIDGE_PATH" ]; then
    echo "Error: Bridge executable not found: $BRIDGE_PATH" >&2
    exit 1
fi

# Set working directory to the directory containing this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

echo "Starting Muehle bridge service on http://${HOST}:${PORT}"

python3 "$SCRIPT_DIR/muehle_bridge_handler.py" \
    --bridge "$BRIDGE_PATH" \
    --host "$HOST" \
    --port "$PORT" \
    --timeout "$TIMEOUT"
