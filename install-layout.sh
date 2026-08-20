#!/bin/bash
# Install the keyboard-quack XKB layout (Vietnamese marks on AltGr as dead keys).
# Uses the standard "custom" layout slot from xkeyboard-config, installed in
# the user's ~/.config/xkb directory - NO SUDO REQUIRED.
set -e

SRC="$(dirname "$0")/config/xkb/symbols/quack"
DEST_DIR="$HOME/.config/xkb/symbols"
DEST="$DEST_DIR/custom"

if [ ! -f "$SRC" ]; then
    echo "Error: cannot find $SRC" >&2
    exit 1
fi

echo "Installing $SRC -> $DEST"
mkdir -p "$DEST_DIR"
cp "$SRC" "$DEST"

echo "Activating the custom layout:"
gsettings set org.gnome.desktop.input-sources sources "[('xkb','custom')]"
gsettings set org.gnome.desktop.input-sources current 0

echo
echo "Done. The custom layout is now active."
echo "  - Base typing = US English (unchanged)"
echo "  - AltGr+s/f/r/x/j/w/z/o/d = Vietnamese marks (used by keyboard-quack)"
echo
echo "If GNOME does not pick it up, log out and back in."
echo "To revert: gsettings set org.gnome.desktop.input-sources sources \"[('xkb','us')]\""