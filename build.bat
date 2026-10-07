@echo off
setlocal
cd /d "%~dp0"

call build-windows-installer.bat
set BUILD_RESULT=%ERRORLEVEL%

echo ========================================================
if %BUILD_RESULT% equ 0 (
    echo     Windows installer build completed
) else (
    echo     Windows installer build failed
)
echo ========================================================
pause
exit /b %BUILD_RESULT%
