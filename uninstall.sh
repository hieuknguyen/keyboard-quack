#!/bin/bash
# ==============================================================================
# keyboard-quack - Uninstaller
# ==============================================================================
set -e

if [ "$EUID" -ne 0 ]; then
    echo "[!] Vui lòng chạy script này với quyền sudo:"
    echo "    sudo ./uninstall.sh"
    exit 1
fi

REAL_USER="${SUDO_USER:-$USER}"
REAL_HOME=$(getent passwd "$REAL_USER" | cut -d: -f6)

echo "=== Đang gỡ bỏ keyboard-quack ==="

# 1. Stop and disable systemd service
if systemctl is-active --quiet keyboard-quack.service 2>/dev/null; then
    systemctl stop keyboard-quack.service
fi
if systemctl is-enabled --quiet keyboard-quack.service 2>/dev/null; then
    systemctl disable keyboard-quack.service
fi
rm -f /etc/systemd/system/keyboard-quack.service
systemctl daemon-reload

# 2. Remove binaries and desktop entry
rm -f /usr/local/bin/quack
rm -f /usr/local/bin/quack-config
rm -f /usr/share/applications/keyboard-quack.desktop
rm -rf /etc/keyboard-quack

# 3. Remove udev rules
rm -f /etc/udev/rules.d/99-keyboard-quack.rules
udevadm control --reload-rules 2>/dev/null || true
udevadm trigger 2>/dev/null || true

# 4. Remove XKB symbols
rm -f /usr/share/X11/xkb/symbols/quack
if [ -n "$REAL_HOME" ]; then
    rm -f "$REAL_HOME/.config/xkb/symbols/custom"
fi

# 5. Revert GNOME input sources to standard US if applicable
if command -v gsettings >/dev/null 2>&1 && [ -n "$SUDO_USER" ]; then
    sudo -u "$REAL_USER" DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$(id -u "$REAL_USER")/bus" \
        gsettings set org.gnome.desktop.input-sources sources "[('xkb','us')]" 2>/dev/null || true
fi

echo "=== Gỡ bỏ hoàn tất thành công! ==="
