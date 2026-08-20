#!/bin/bash
set -e

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Install/activate the XKB layout in the real desktop user's session.
bash "$ROOT_DIR/install-layout.sh"

# Run the daemon with the freshly activated layout.
exec sudo -E "$ROOT_DIR/build/quack" "$@"
