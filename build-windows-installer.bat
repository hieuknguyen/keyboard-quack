@echo off
setlocal
cd /d "%~dp0"

if exist "C:\msys64\ucrt64\bin\gcc.exe" set "PATH=C:\msys64\ucrt64\bin;%PATH%"

where gcc >nul 2>nul || goto :missing_tools
where windres >nul 2>nul || goto :missing_tools

if not exist build-windows mkdir build-windows
if not exist build-windows\installer-payload mkdir build-windows\installer-payload

echo [1/3] Building installed application payload...
gcc -Wall -Wextra -O2 -std=c11 -mwindows -Isrc -o build-windows\installer-payload\quack.exe src\main.c src\platform\platform.c src\engine\telex.c src\capture\win32_capture.c src\capture\win32_uia.c src\windows\win_update.c src\inject\win32_inject.c src\config\config.c -luser32 -lshell32 -ladvapi32 -luiautomationcore -lole32 -loleaut32 -luuid -lwinhttp -lbcrypt
if errorlevel 1 goto :failed

gcc -Wall -Wextra -O2 -std=c11 -mwindows -Isrc\windows -o build-windows\installer-payload\uninstall.exe src\windows\uninstaller.c -luser32 -lshell32 -ladvapi32
if errorlevel 1 goto :failed

echo [2/3] Embedding application and uninstaller...
windres -Isrc\windows -i src\windows\installer_resources.rc -o build-windows\installer_resources.o
if errorlevel 1 goto :failed

echo [3/3] Building self-contained setup application...
gcc -Wall -Wextra -O2 -std=c11 -mwindows -Isrc\windows -o build-windows\keyboard-quack-setup.exe src\windows\installer.c build-windows\installer_resources.o -luser32 -lshell32 -ladvapi32 -lole32 -loleaut32 -luuid
if errorlevel 1 goto :failed

echo.
echo [OK] Installer created: build-windows\keyboard-quack-setup.exe
exit /b 0

:missing_tools
echo [!] MinGW-w64 GCC and windres are required. Install the MSYS2 UCRT64 toolchain or add it to PATH.
exit /b 1

:failed
echo [!] Installer build failed.
exit /b 1
