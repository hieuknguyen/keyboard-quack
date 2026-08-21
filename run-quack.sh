#!/bin/bash
set -e

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Install/activate the XKB layout in the real desktop user's session.
bash "$ROOT_DIR/install-layout.sh"

# Run the daemon with the freshly activated layout.
if [ -f "$ROOT_DIR/build/quack" ]; then
    EXEC_BIN="$ROOT_DIR/build/quack"
elif [ -f "$ROOT_DIR/quack" ]; then
    EXEC_BIN="$ROOT_DIR/quack"
else
    echo "Error: quack binary not found. Please run 'make' first." >&2
    exit 1
fi

exec sudo -E "$EXEC_BIN" "$@"
