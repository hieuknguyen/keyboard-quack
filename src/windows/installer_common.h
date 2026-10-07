#ifndef QUACK_WINDOWS_INSTALLER_COMMON_H
#define QUACK_WINDOWS_INSTALLER_COMMON_H

#define QUACK_INSTALLED_APP_NAME L"keyboard-quack"
#define QUACK_INSTALLED_APP_EXE L"quack.exe"
#define QUACK_INSTALLED_UNINSTALLER_EXE L"uninstall.exe"
#define QUACK_VERSION_A "1.0.4"
#define QUACK_VERSION_W L"1.0.4"
#define QUACK_STARTUP_REGISTRY_KEY \
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define QUACK_STARTUP_REGISTRY_VALUE L"keyboard-quack"
#define QUACK_UNINSTALL_REGISTRY_KEY \
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\keyboard-quack"
#define QUACK_APP_WINDOW_CLASS L"KeyboardQuackHiddenWindow"
#define QUACK_APP_WINDOW_TITLE L"keyboard-quack"

#define IDR_QUACK_INSTALLED_EXE 101
#define IDR_QUACK_UNINSTALLER_EXE 102

#endif
