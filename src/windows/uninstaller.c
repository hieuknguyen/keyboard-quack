#define COBJMACROS
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "installer_common.h"

static bool g_quiet = false;

static void show_error(const WCHAR *operation, DWORD error)
{
    if (g_quiet) return;
    WCHAR message[256];
    swprintf(message, sizeof(message) / sizeof(message[0]),
             L"%ls failed (Windows error %lu).",
             operation, (unsigned long)error);
    MessageBoxW(NULL, message, L"Uninstall keyboard-quack",
                MB_OK | MB_ICONERROR);
}

static bool get_module_path(WCHAR *path, DWORD capacity)
{
    DWORD length = GetModuleFileNameW(NULL, path, capacity);
    if (length == 0 || length >= capacity) return false;
    path[length] = L'\0';
    return true;
}

static void remove_trailing_separator(WCHAR *path)
{
    size_t length = wcslen(path);
    while (length > 3 &&
           (path[length - 1] == L'\\' || path[length - 1] == L'/')) {
        path[--length] = L'\0';
    }
}

static bool validate_install_location(WCHAR *install_dir, size_t capacity)
{
    HKEY key = NULL;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER,
                                QUACK_UNINSTALL_REGISTRY_KEY, 0,
                                KEY_QUERY_VALUE, &key);
    if (result != ERROR_SUCCESS) return false;

    DWORD type = 0;
    DWORD bytes = (DWORD)(capacity * sizeof(WCHAR));
    result = RegQueryValueExW(key, L"InstallLocation", NULL, &type,
                              (BYTE *)install_dir, &bytes);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS || type != REG_SZ || bytes < sizeof(WCHAR))
        return false;
    install_dir[capacity - 1] = L'\0';

    WCHAR module_path[MAX_PATH];
    if (!get_module_path(module_path, MAX_PATH)) return false;
    WCHAR *last_separator = wcsrchr(module_path, L'\\');
    if (!last_separator) return false;
    *last_separator = L'\0';

    remove_trailing_separator(install_dir);
    remove_trailing_separator(module_path);
    return _wcsicmp(install_dir, module_path) == 0;
}

static DWORD remove_startup_entry(void)
{
    HKEY key = NULL;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER,
                                QUACK_STARTUP_REGISTRY_KEY, 0,
                                KEY_SET_VALUE, &key);
    if (result == ERROR_FILE_NOT_FOUND) return ERROR_SUCCESS;
    if (result != ERROR_SUCCESS) return (DWORD)result;

    result = RegDeleteValueW(key, QUACK_STARTUP_REGISTRY_VALUE);
    RegCloseKey(key);
    return result == ERROR_FILE_NOT_FOUND ? ERROR_SUCCESS : (DWORD)result;
}

static DWORD remove_uninstall_entry(void)
{
    LONG result = RegDeleteKeyW(HKEY_CURRENT_USER,
                                QUACK_UNINSTALL_REGISTRY_KEY);
    return result == ERROR_FILE_NOT_FOUND ? ERROR_SUCCESS : (DWORD)result;
}

static void remove_start_menu_shortcuts(void)
{
    WCHAR programs[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS | CSIDL_FLAG_CREATE,
                                NULL, SHGFP_TYPE_CURRENT, programs))) {
        return;
    }

    WCHAR folder[MAX_PATH];
    if (swprintf(folder, MAX_PATH, L"%ls\\%ls", programs,
                 QUACK_INSTALLED_APP_NAME) < 0) {
        return;
    }

    WCHAR shortcut[MAX_PATH];
    if (swprintf(shortcut, MAX_PATH, L"%ls\\keyboard-quack.lnk", folder) >= 0)
        DeleteFileW(shortcut);
    if (swprintf(shortcut, MAX_PATH,
                  L"%ls\\Uninstall keyboard-quack.lnk", folder) >= 0)
        DeleteFileW(shortcut);
    RemoveDirectoryW(folder);
}

static bool close_running_app(void)
{
    HWND app_window = FindWindowW(QUACK_APP_WINDOW_CLASS,
                                  QUACK_APP_WINDOW_TITLE);
    if (!app_window) return true;

    DWORD process_id = 0;
    GetWindowThreadProcessId(app_window, &process_id);
    if (!process_id || process_id == GetCurrentProcessId()) return true;

    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, process_id);
    PostMessageW(app_window, WM_CLOSE, 0, 0);
    if (!process) {
        for (int i = 0; i < 50 && IsWindow(app_window); i++) Sleep(100);
        return !IsWindow(app_window);
    }

    DWORD wait = WaitForSingleObject(process, 5000);
    CloseHandle(process);
    return wait == WAIT_OBJECT_0;
}

static DWORD schedule_file_delete(const WCHAR *path)
{
    if (DeleteFileW(path) || GetLastError() == ERROR_FILE_NOT_FOUND)
        return ERROR_SUCCESS;
    if (MoveFileExW(path, NULL, MOVEFILE_DELAY_UNTIL_REBOOT))
        return ERROR_SUCCESS;
    return GetLastError();
}

static DWORD schedule_install_folder_cleanup(const WCHAR *install_dir,
                                             bool *restart_required)
{
    WCHAR app_path[MAX_PATH];
    WCHAR uninstaller_path[MAX_PATH];
    if (swprintf(app_path, MAX_PATH, L"%ls\\%ls", install_dir,
                 QUACK_INSTALLED_APP_EXE) < 0 ||
        swprintf(uninstaller_path, MAX_PATH, L"%ls\\%ls", install_dir,
                 QUACK_INSTALLED_UNINSTALLER_EXE) < 0) {
        return ERROR_BUFFER_OVERFLOW;
    }

    if (restart_required) *restart_required = false;

    size_t dir_length = wcslen(install_dir);
    size_t escaped_length = dir_length;
    for (size_t i = 0; i < dir_length; i++) {
        if (install_dir[i] == L'\'') escaped_length++;
    }
    WCHAR *escaped_dir = (WCHAR *)malloc((escaped_length + 1) * sizeof(WCHAR));
    if (!escaped_dir) return ERROR_NOT_ENOUGH_MEMORY;

    size_t write_at = 0;
    for (size_t i = 0; i < dir_length; i++) {
        escaped_dir[write_at++] = install_dir[i];
        if (install_dir[i] == L'\'') escaped_dir[write_at++] = L'\'';
    }
    escaped_dir[write_at] = L'\0';

    WCHAR system_dir[MAX_PATH];
    UINT system_length = GetSystemDirectoryW(system_dir, MAX_PATH);
    WCHAR powershell_path[MAX_PATH + 64];
    int ps_length = -1;
    if (system_length > 0 && system_length < MAX_PATH) {
        ps_length = swprintf(powershell_path,
                             sizeof(powershell_path) /
                                 sizeof(powershell_path[0]),
                             L"%ls\\WindowsPowerShell\\v1.0\\powershell.exe",
                             system_dir);
    }

    DWORD result = ERROR_FILE_NOT_FOUND;
    if (ps_length >= 0 &&
        (size_t)ps_length < sizeof(powershell_path) /
                                sizeof(powershell_path[0])) {
        size_t command_capacity = escaped_length + (size_t)ps_length + 1024;
        WCHAR *command_line = (WCHAR *)malloc(command_capacity * sizeof(WCHAR));
        if (command_line) {
            int command_length = swprintf(
                command_line, command_capacity,
                L"\"%ls\" -NoProfile -NonInteractive -WindowStyle Hidden "
                L"-Command \"$d='%ls'; for ($i=0; $i -lt 40; $i++) { "
                L"try { foreach ($n in 'quack.exe','uninstall.exe') { "
                L"$p=Join-Path $d $n; if (Test-Path -LiteralPath $p) { "
                L"Remove-Item -LiteralPath $p -Force -ErrorAction Stop } }; "
                L"if ([IO.Directory]::GetFileSystemEntries($d).Length -eq 0) "
                L"{ [IO.Directory]::Delete($d) }; exit 0 } "
                L"catch { Start-Sleep -Milliseconds 250 } }\"",
                powershell_path, escaped_dir);

            if (command_length >= 0 &&
                (size_t)command_length < command_capacity) {
                STARTUPINFOW startup_info;
                PROCESS_INFORMATION process_info;
                ZeroMemory(&startup_info, sizeof(startup_info));
                ZeroMemory(&process_info, sizeof(process_info));
                startup_info.cb = sizeof(startup_info);
                if (CreateProcessW(powershell_path, command_line, NULL, NULL,
                                   FALSE, CREATE_NO_WINDOW, NULL, NULL,
                                   &startup_info, &process_info)) {
                    CloseHandle(process_info.hThread);
                    CloseHandle(process_info.hProcess);
                    result = ERROR_SUCCESS;
                } else {
                    result = GetLastError();
                }
            } else {
                result = ERROR_BUFFER_OVERFLOW;
            }
            free(command_line);
        } else {
            result = ERROR_NOT_ENOUGH_MEMORY;
        }
    } else if (system_length == 0) {
        result = GetLastError();
    } else {
        result = ERROR_BUFFER_OVERFLOW;
    }

    free(escaped_dir);

    /* The helper retries after this process exits, so avoid scheduling a
     * reboot-time deletion when it started successfully. */
    if (result == ERROR_SUCCESS) return ERROR_SUCCESS;

    DWORD app_delete = schedule_file_delete(app_path);
    DWORD uninstaller_delete = MoveFileExW(uninstaller_path, NULL,
                                           MOVEFILE_DELAY_UNTIL_REBOOT)
                                   ? ERROR_SUCCESS : GetLastError();
    if (app_delete == ERROR_SUCCESS || uninstaller_delete == ERROR_SUCCESS) {
        if (restart_required) *restart_required = true;
        return ERROR_SUCCESS;
    }
    return result;
}

static int uninstall_application(const WCHAR *install_dir)
{
    if (!g_quiet) {
        int answer = MessageBoxW(
            NULL,
            L"This will remove keyboard-quack, its Start Menu shortcuts, and "
            L"its Windows startup entry. Your settings in AppData will be kept. "
            L"Continue?",
            L"Uninstall keyboard-quack",
            MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
        if (answer != IDYES) return 0;
    }

    DWORD error = remove_startup_entry();
    if (error != ERROR_SUCCESS) {
        show_error(L"Removing the startup entry", error);
        return 1;
    }

    if (!close_running_app()) {
        show_error(L"Closing the running application", ERROR_TIMEOUT);
        return 1;
    }

    remove_start_menu_shortcuts();
    bool restart_required = false;
    error = schedule_install_folder_cleanup(install_dir, &restart_required);
    if (error != ERROR_SUCCESS) {
        show_error(L"Scheduling installed files for removal", error);
        return 1;
    }

    error = remove_uninstall_entry();
    if (error != ERROR_SUCCESS) {
        show_error(L"Removing the uninstall registration", error);
        return 1;
    }

    if (restart_required && !g_quiet) {
        MessageBoxW(NULL,
                    L"The app has been unregistered. Windows will finish "
                    L"removing its files during the next restart if they "
                    L"remain locked.",
                    L"Uninstall scheduled", MB_OK | MB_ICONINFORMATION);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous_instance,
                   LPSTR command_line, int show_command)
{
    (void)instance;
    (void)previous_instance;
    (void)command_line;
    (void)show_command;

    WCHAR install_dir[MAX_PATH];
    if (!validate_install_location(install_dir,
                                   sizeof(install_dir) / sizeof(install_dir[0]))) {
        MessageBoxW(NULL,
                    L"This copy is not registered as an installed "
                    L"keyboard-quack application.",
                    L"Uninstall keyboard-quack", MB_OK | MB_ICONERROR);
        return 1;
    }

    int argc = 0;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int i = 1; argv && i < argc; i++) {
        if (_wcsicmp(argv[i], L"/quiet") == 0 ||
            _wcsicmp(argv[i], L"--quiet") == 0) {
            g_quiet = true;
        }
    }
    if (argv) LocalFree(argv);

    return uninstall_application(install_dir);
}
