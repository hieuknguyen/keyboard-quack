#!/bin/bash
# ==============================================================================
# keyboard-quack - Automated 1-Click Installer for Linux
# ==============================================================================
set -e

# Ensure script is run with sudo/root privileges
if [ "$EUID" -ne 0 ]; then
    echo "[!] Vui lòng chạy script này với quyền sudo:"
    echo "    sudo ./install.sh"
    exit 1
fi

REAL_USER="${SUDO_USER:-$USER}"
REAL_HOME=$(getent passwd "$REAL_USER" | cut -d: -f6)
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "========================================================"
echo "    Bắt đầu cài đặt keyboard-quack cho Linux"
echo "    User: $REAL_USER ($REAL_HOME)"
echo "========================================================"

# 1. Build binaries if needed
echo "[1/6] Kiểm tra và biên dịch mã nguồn..."
mkdir -p "$SCRIPT_DIR/build"
cd "$SCRIPT_DIR/build"
if command -v cmake >/dev/null 2>&1; then
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j$(nproc)
else
    cd "$SCRIPT_DIR"
    make
fi
cd "$SCRIPT_DIR"

# 2. Install binaries to /usr/local/bin
echo "[2/6] Cài đặt file thực thi vào /usr/local/bin..."
if [ -f "$SCRIPT_DIR/build/quack" ]; then
    install -m 755 "$SCRIPT_DIR/build/quack" /usr/local/bin/quack
elif [ -f "$SCRIPT_DIR/quack" ]; then
    install -m 755 "$SCRIPT_DIR/quack" /usr/local/bin/quack
fi
if [ -f "$SCRIPT_DIR/build/quack-config" ]; then
    install -m 755 "$SCRIPT_DIR/build/quack-config" /usr/local/bin/quack-config
fi

# 3. Install default configuration
echo "[3/6] Cài đặt file cấu hình..."
mkdir -p /etc/keyboard-quack
install -m 644 "$SCRIPT_DIR/config/default.toml" /etc/keyboard-quack/config.toml

# 4. Install udev rules & add user to input group
echo "[4/6] Thiết lập quyền truy cập thiết bị (udev rules & input group)..."
cat > /etc/udev/rules.d/99-keyboard-quack.rules << 'UDEV_EOF'
KERNEL=="uinput", GROUP="input", MODE="0660"
KERNEL=="event*", SUBSYSTEM=="input", GROUP="input", MODE="0660"
UDEV_EOF

udevadm control --reload-rules 2>/dev/null || true
udevadm trigger 2>/dev/null || true

if getent group input >/dev/null; then
    usermod -aG input "$REAL_USER"
fi

# 5. Install XKB Layout & Desktop Entry
echo "[5/6] Cài đặt XKB symbols và Desktop Entry..."
# System-wide XKB symbols
if [ -d /usr/share/X11/xkb/symbols ]; then
    cp "$SCRIPT_DIR/config/xkb/symbols/quack" /usr/share/X11/xkb/symbols/quack
    cp "$SCRIPT_DIR/config/xkb/symbols/quack" /usr/share/X11/xkb/symbols/custom
fi

# User-space XKB symbol
if [ -n "$REAL_HOME" ]; then
    USER_XKB_DIR="$REAL_HOME/.config/xkb/symbols"
    mkdir -p "$USER_XKB_DIR"
    cp "$SCRIPT_DIR/config/xkb/symbols/quack" "$USER_XKB_DIR/custom"
    chown -R "$REAL_USER:$REAL_USER" "$REAL_HOME/.config/xkb" 2>/dev/null || true
fi

# Activate layout for GNOME session if applicable
if command -v gsettings >/dev/null 2>&1 && [ -n "$SUDO_USER" ]; then
    sudo -u "$REAL_USER" DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$(id -u "$REAL_USER")/bus" \
        gsettings set org.gnome.desktop.input-sources sources "[('xkb','custom')]" 2>/dev/null || true
fi

# Desktop launcher entry
mkdir -p /usr/share/applications
cat > /usr/share/applications/keyboard-quack.desktop << 'DESKTOP_EOF'
[Desktop Entry]
Name=keyboard-quack
Comment=Bộ gõ tiếng Việt Telex cho Linux
Exec=quack-config
Icon=input-keyboard
Terminal=false
Type=Application
Categories=Utility;Settings;
Keywords=vietnamese;input;telex;keyboard;
DESKTOP_EOF

# 6. Install and enable systemd service
echo "[6/6] Đăng ký Systemd Daemon Service..."
cat > /etc/systemd/system/keyboard-quack.service << 'SERVICE_EOF'
[Unit]
Description=keyboard-quack Vietnamese Input Daemon
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/quack
Restart=always
RestartSec=2
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
SERVICE_EOF

systemctl daemon-reload
systemctl enable --now keyboard-quack.service

echo
echo "========================================================"
echo "    CÀI ĐẶT HOÀN TẤT THÀNH CÔNG!"
echo "========================================================"
echo "  - Daemon đã được kích hoạt và chạy tự động cùng hệ thống."
echo "  - Phím tắt Bật/Tắt tiếng Việt: Ctrl + Space"
echo "  - Mở giao diện cài đặt: quack-config (hoặc tìm trong Menu App)"
echo "  - Để gỡ cài đặt: sudo ./uninstall.sh"
echo "========================================================"
