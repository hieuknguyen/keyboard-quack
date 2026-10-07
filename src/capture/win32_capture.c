#if defined(_WIN32) || defined(_WIN64)

#include "win32_capture.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "../windows/installer_common.h"
#include "../windows/win_update.h"

static win32_capture_ctx_t *g_capture_ctx = NULL;

#define QUACK_TRAY_MESSAGE (WM_APP + 1)
#define QUACK_TRAY_TOGGLE  1001
#define QUACK_TRAY_EXIT    1002
#define QUACK_TRAY_STARTUP 1003
#define QUACK_TRAY_UNINSTALL 1004
#define QUACK_TRAY_UPDATE 1005
#define UIA_SNAPSHOT_MAX_AGE_MS 500
#define UIA_CARET_SETTLE_MS     75

static const char *QUACK_WINDOW_CLASS = "KeyboardQuackHiddenWindow";
static const WCHAR *QUACK_STARTUP_KEY =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const WCHAR *QUACK_STARTUP_VALUE = L"keyboard-quack";

static DWORD get_startup_enabled(bool *enabled)
{
    if (!enabled) return ERROR_INVALID_PARAMETER;
    *enabled = false;

    HKEY key = NULL;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, QUACK_STARTUP_KEY, 0,
                                KEY_QUERY_VALUE, &key);
    if (result == ERROR_FILE_NOT_FOUND) return ERROR_SUCCESS;
    if (result != ERROR_SUCCESS) return (DWORD)result;

    DWORD type = 0;
    result = RegQueryValueExW(key, QUACK_STARTUP_VALUE, NULL, &type, NULL, NULL);
    if (result == ERROR_SUCCESS) {
        *enabled = true;
    } else if (result == ERROR_FILE_NOT_FOUND) {
        result = ERROR_SUCCESS;
    }

    RegCloseKey(key);
    return (DWORD)result;
}

static WCHAR *get_current_executable_path(DWORD *path_length,
                                          DWORD *error_out)
{
    DWORD capacity = MAX_PATH;
    WCHAR *path = NULL;

    if (error_out) *error_out = ERROR_SUCCESS;
    for (;;) {
        WCHAR *next = (WCHAR *)realloc(path,
                                       (size_t)capacity * sizeof(WCHAR));
        if (!next) {
            free(path);
            if (error_out) *error_out = ERROR_NOT_ENOUGH_MEMORY;
            return NULL;
        }
        path = next;

        DWORD length = GetModuleFileNameW(NULL, path, capacity);
        if (length == 0) {
            DWORD error = GetLastError();
            free(path);
            if (error_out) *error_out = error ? error : ERROR_GEN_FAILURE;
            return NULL;
        }
        if (length < capacity) {
            if (path_length) *path_length = length;
            return path;
        }
        if (capacity >= 32768) {
            free(path);
            if (error_out) *error_out = ERROR_BUFFER_OVERFLOW;
            return NULL;
        }
        capacity = capacity > 16384 ? 32768 : capacity * 2;
    }
}

static DWORD set_startup_enabled(bool enabled)
{
    HKEY key = NULL;
    LONG result;

    if (!enabled) {
        result = RegOpenKeyExW(HKEY_CURRENT_USER, QUACK_STARTUP_KEY, 0,
                               KEY_SET_VALUE, &key);
        if (result == ERROR_FILE_NOT_FOUND) return ERROR_SUCCESS;
        if (result != ERROR_SUCCESS) return (DWORD)result;

        result = RegDeleteValueW(key, QUACK_STARTUP_VALUE);
        RegCloseKey(key);
        return result == ERROR_FILE_NOT_FOUND ? ERROR_SUCCESS : (DWORD)result;
    }

    DWORD path_length = 0;
    DWORD path_error = ERROR_SUCCESS;
    WCHAR *executable_path = get_current_executable_path(&path_length,
                                                          &path_error);
    if (!executable_path) return path_error;

    size_t command_chars = (size_t)path_length + 3;
    WCHAR *command = (WCHAR *)malloc(command_chars * sizeof(WCHAR));
    if (!command) {
        free(executable_path);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    command[0] = L'"';
    memcpy(command + 1, executable_path,
           (size_t)path_length * sizeof(WCHAR));
    command[path_length + 1] = L'"';
    command[path_length + 2] = L'\0';
    free(executable_path);

    DWORD disposition = 0;
    result = RegCreateKeyExW(HKEY_CURRENT_USER, QUACK_STARTUP_KEY, 0, NULL, 0,
                             KEY_SET_VALUE, NULL, &key, &disposition);
    if (result == ERROR_SUCCESS) {
        DWORD bytes = (DWORD)(command_chars * sizeof(WCHAR));
        result = RegSetValueExW(key, QUACK_STARTUP_VALUE, 0, REG_SZ,
                                (const BYTE *)command, bytes);
        RegCloseKey(key);
    }
    free(command);
    return (DWORD)result;
}

static DWORD schedule_executable_removal(bool *restart_required)
{
    DWORD path_length = 0;
    DWORD error = ERROR_SUCCESS;
    WCHAR *executable_path = get_current_executable_path(&path_length,
                                                          &error);
    if (!executable_path) return error;
    if (restart_required) *restart_required = false;

    size_t escaped_length = (size_t)path_length;
    for (DWORD i = 0; i < path_length; i++) {
        if (executable_path[i] == L'\'') escaped_length++;
    }
    WCHAR *escaped_path = (WCHAR *)malloc((escaped_length + 1) * sizeof(WCHAR));
    if (!escaped_path) {
        free(executable_path);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    size_t write_at = 0;
    for (DWORD i = 0; i < path_length; i++) {
        escaped_path[write_at++] = executable_path[i];
        if (executable_path[i] == L'\'') {
            escaped_path[write_at++] = L'\'';
        }
    }
    escaped_path[write_at] = L'\0';

    WCHAR system_dir[MAX_PATH];
    UINT system_length = GetSystemDirectoryW(system_dir, MAX_PATH);
    if (system_length == 0 || system_length >= MAX_PATH) {
        error = GetLastError();
        if (!error) error = ERROR_BUFFER_OVERFLOW;
        goto fallback;
    }

    WCHAR powershell_path[MAX_PATH + 64];
    int ps_length = swprintf(powershell_path,
                             sizeof(powershell_path) / sizeof(powershell_path[0]),
                             L"%ls\\WindowsPowerShell\\v1.0\\powershell.exe",
                             system_dir);
    if (ps_length < 0 || (size_t)ps_length >=
                         sizeof(powershell_path) / sizeof(powershell_path[0])) {
        error = ERROR_BUFFER_OVERFLOW;
        goto fallback;
    }

    size_t command_capacity = escaped_length + (size_t)ps_length + 512;
    WCHAR *command_line = (WCHAR *)malloc(command_capacity * sizeof(WCHAR));
    if (!command_line) {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto fallback;
    }

    int command_length = swprintf(
        command_line, command_capacity,
        L"\"%ls\" -NoProfile -NonInteractive -WindowStyle Hidden "
        L"-Command \"for ($i=0; $i -lt 40; $i++) { try { "
        L"Remove-Item -LiteralPath '%ls' -Force -ErrorAction Stop; "
        L"exit 0 } catch { Start-Sleep -Milliseconds 250 } }\"",
        powershell_path, escaped_path);
    if (command_length < 0 || (size_t)command_length >= command_capacity) {
        free(command_line);
        error = ERROR_BUFFER_OVERFLOW;
        goto fallback;
    }

    STARTUPINFOW startup_info;
    PROCESS_INFORMATION process_info;
    ZeroMemory(&startup_info, sizeof(startup_info));
    ZeroMemory(&process_info, sizeof(process_info));
    startup_info.cb = sizeof(startup_info);
    BOOL created = CreateProcessW(powershell_path, command_line, NULL, NULL,
                                  FALSE, CREATE_NO_WINDOW, NULL, NULL,
                                  &startup_info, &process_info);
    free(command_line);
    if (created) {
        CloseHandle(process_info.hThread);
        CloseHandle(process_info.hProcess);
        if (MoveFileExW(executable_path, NULL, MOVEFILE_DELAY_UNTIL_REBOOT) &&
            restart_required) {
            *restart_required = true;
        }
        free(escaped_path);
        free(executable_path);
        return ERROR_SUCCESS;
    }

    error = GetLastError();

fallback:
    free(escaped_path);
    if (MoveFileExW(executable_path, NULL, MOVEFILE_DELAY_UNTIL_REBOOT)) {
        free(executable_path);
        if (restart_required) *restart_required = true;
        return ERROR_SUCCESS;
    }
    error = GetLastError();
    free(executable_path);
    return error ? error : ERROR_GEN_FAILURE;
}

static void show_startup_error(HWND hwnd, DWORD error)
{
    WCHAR message[160];
    swprintf(message, sizeof(message) / sizeof(message[0]),
             L"Could not update the Windows startup setting (error %lu).",
             (unsigned long)error);
    MessageBoxW(hwnd, message, L"keyboard-quack", MB_OK | MB_ICONERROR);
}

static void show_uninstall_error(HWND hwnd, DWORD error)
{
    WCHAR message[160];
    swprintf(message, sizeof(message) / sizeof(message[0]),
             L"Could not remove this EXE (Windows error %lu).",
             (unsigned long)error);
    MessageBoxW(hwnd, message, L"Uninstall keyboard-quack",
                MB_OK | MB_ICONERROR);
}

/* A packaged install has a dedicated uninstaller; portable copies keep the
 * self-removal flow below. Only launch it when this EXE is in the registered
 * install directory, so another portable copy cannot uninstall the package. */
static bool launch_installed_uninstaller(HWND hwnd)
{
    HKEY key = NULL;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER,
                                QUACK_UNINSTALL_REGISTRY_KEY, 0,
                                KEY_QUERY_VALUE, &key);
    if (result != ERROR_SUCCESS) return false;

    WCHAR registered_dir[MAX_PATH];
    DWORD type = 0;
    DWORD bytes = sizeof(registered_dir);
    result = RegQueryValueExW(key, L"InstallLocation", NULL, &type,
                              (BYTE *)registered_dir, &bytes);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS || type != REG_SZ || bytes < sizeof(WCHAR))
        return false;
    registered_dir[MAX_PATH - 1] = L'\0';

    WCHAR *module_path = get_current_executable_path(NULL, NULL);
    if (!module_path) return false;
    WCHAR *last_separator = wcsrchr(module_path, L'\\');
    if (!last_separator) {
        free(module_path);
        return false;
    }
    *last_separator = L'\0';
    while (wcslen(registered_dir) > 3 &&
           (registered_dir[wcslen(registered_dir) - 1] == L'\\' ||
            registered_dir[wcslen(registered_dir) - 1] == L'/')) {
        registered_dir[wcslen(registered_dir) - 1] = L'\0';
    }

    bool is_installed_copy = _wcsicmp(module_path, registered_dir) == 0;
    free(module_path);
    if (!is_installed_copy) return false;

    WCHAR uninstaller_path[MAX_PATH];
    if (swprintf(uninstaller_path, MAX_PATH, L"%ls\\%ls", registered_dir,
                 QUACK_INSTALLED_UNINSTALLER_EXE) < 0) {
        show_uninstall_error(hwnd, ERROR_BUFFER_OVERFLOW);
        return true;
    }
    if (GetFileAttributesW(uninstaller_path) == INVALID_FILE_ATTRIBUTES) {
        show_uninstall_error(hwnd, ERROR_FILE_NOT_FOUND);
        return true;
    }

    WCHAR command_line[MAX_PATH + 4];
    int command_length = swprintf(command_line, MAX_PATH + 4, L"\"%ls\"",
                                  uninstaller_path);
    if (command_length < 0 || command_length >= MAX_PATH + 4) {
        show_uninstall_error(hwnd, ERROR_BUFFER_OVERFLOW);
        return true;
    }

    STARTUPINFOW startup_info;
    PROCESS_INFORMATION process_info;
    ZeroMemory(&startup_info, sizeof(startup_info));
    ZeroMemory(&process_info, sizeof(process_info));
    startup_info.cb = sizeof(startup_info);
    if (!CreateProcessW(uninstaller_path, command_line, NULL, NULL, FALSE, 0,
                        NULL, registered_dir, &startup_info, &process_info)) {
        show_uninstall_error(hwnd, GetLastError());
        return true;
    }

    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);
    win32_capture_stop(g_capture_ctx);
    return true;
}

static void uninstall_current_copy(HWND hwnd)
{
    if (launch_installed_uninstaller(hwnd)) return;

    int answer = MessageBoxW(
        hwnd,
        L"This will remove keyboard-quack from Windows startup and delete "
        L"the currently running EXE. Other copies and your settings will be "
        L"kept. Continue?",
        L"Uninstall keyboard-quack", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
    if (answer != IDYES) return;

    bool startup_enabled = false;
    DWORD error = get_startup_enabled(&startup_enabled);
    if (error != ERROR_SUCCESS) {
        show_startup_error(hwnd, error);
        return;
    }

    error = set_startup_enabled(false);
    if (error != ERROR_SUCCESS) {
        show_startup_error(hwnd, error);
        return;
    }

    bool restart_required = false;
    error = schedule_executable_removal(&restart_required);
    if (error != ERROR_SUCCESS) {
        if (startup_enabled) (void)set_startup_enabled(true);
        show_uninstall_error(hwnd, error);
        return;
    }

    if (restart_required) {
        MessageBoxW(hwnd,
                    L"The startup entry was removed. This EXE will be removed "
                    L"after the app closes; a restart may be needed to finish. "
                    L"Your settings and other copies are kept.",
                    L"Uninstall scheduled", MB_OK | MB_ICONINFORMATION);
    }
    win32_capture_stop(g_capture_ctx);
}

static bool uia_same_element(const win32_uia_snapshot_t *snapshot,
                             const win32_capture_ctx_t *ctx)
{
    if (snapshot->process_id != ctx->uia_process_id ||
        snapshot->element_window != ctx->uia_element_window) {
        return false;
    }
    return !snapshot->element_runtime_id || !ctx->uia_element_runtime_id ||
           snapshot->element_runtime_id == ctx->uia_element_runtime_id;
}

static void uia_remember_snapshot(win32_capture_ctx_t *ctx,
                                  const win32_uia_snapshot_t *snapshot)
{
    ctx->uia_process_id = snapshot->process_id;
    ctx->uia_element_window = snapshot->element_window;
    ctx->uia_element_runtime_id = snapshot->element_runtime_id;
    ctx->uia_expected_caret = snapshot->caret_offset;
    ctx->uia_caret_valid = true;
    ctx->uia_mismatch_since_ms = 0;
}

static void uia_invalidate_caret(win32_capture_ctx_t *ctx)
{
    ctx->uia_caret_valid = false;
    ctx->uia_selection_latched = false;
    ctx->uia_invalidated_at_ms = GetTickCount64();
    ctx->uia_mismatch_since_ms = 0;
}

/* UIA may be unavailable in the newly focused app, so track app focus directly. */
static void reset_on_foreground_change(win32_capture_ctx_t *ctx)
{
    HWND foreground = GetForegroundWindow();
    DWORD process_id = 0;
    if (foreground) {
        GetWindowThreadProcessId(foreground, &process_id);
    }

    if (!ctx->foreground_identity_initialized) {
        ctx->foreground_window = foreground;
        ctx->foreground_process_id = process_id;
        ctx->foreground_identity_initialized = true;
        return;
    }

    if (foreground == ctx->foreground_window &&
        process_id == ctx->foreground_process_id) {
        return;
    }

    telex_reset_tracking(ctx->tctx);
    uia_invalidate_caret(ctx);
    ctx->foreground_window = foreground;
    ctx->foreground_process_id = process_id;
}

/*
 * Reconcile composition state against a recent UIA caret snapshot. UIA runs on
 * its own worker; this only copies a small snapshot while inside the key hook.
 */
static void uia_reconcile_before_key(win32_capture_ctx_t *ctx)
{
    win32_uia_snapshot_t snapshot;
    ULONGLONG now = GetTickCount64();

    if (!win32_uia_get_snapshot(&ctx->uia, &snapshot) || !snapshot.available ||
        snapshot.sampled_at_ms == 0 ||
        now - snapshot.sampled_at_ms > UIA_SNAPSHOT_MAX_AGE_MS ||
        snapshot.sampled_at_ms <= ctx->uia_last_sample_ms) {
        return;
    }

    ctx->uia_last_sample_ms = snapshot.sampled_at_ms;

    if (snapshot.has_selection) {
        if (!ctx->uia_selection_latched) {
            telex_reset_tracking(ctx->tctx);
            ctx->uia_caret_valid = false;
            ctx->uia_selection_latched = true;
            ctx->uia_invalidated_at_ms = snapshot.sampled_at_ms;
        }
        return;
    }

    if (ctx->uia_selection_latched) {
        ctx->uia_selection_latched = false;
        ctx->uia_caret_valid = false;
    }

    if (!ctx->uia_caret_valid) {
        if (snapshot.sampled_at_ms > ctx->uia_invalidated_at_ms) {
            uia_remember_snapshot(ctx, &snapshot);
        }
        return;
    }

    if (!uia_same_element(&snapshot, ctx)) {
        telex_reset_tracking(ctx->tctx);
        uia_remember_snapshot(ctx, &snapshot);
        return;
    }

    if (snapshot.caret_offset == ctx->uia_expected_caret) {
        ctx->uia_mismatch_since_ms = 0;
        return;
    }

    if (ctx->uia_mismatch_since_ms == 0 ||
        ctx->uia_mismatch_caret != snapshot.caret_offset) {
        ctx->uia_mismatch_since_ms = now;
        ctx->uia_mismatch_caret = snapshot.caret_offset;
        return;
    }

    /* Ignore transient provider lag immediately after our own injected text. */
    if (now - ctx->uia_mismatch_since_ms >= UIA_CARET_SETTLE_MS &&
        now - ctx->uia_last_input_ms >= UIA_CARET_SETTLE_MS) {
        telex_reset_tracking(ctx->tctx);
        uia_remember_snapshot(ctx, &snapshot);
    }
}

static int output_utf16_length(const uint32_t *text, int length)
{
    int units = 0;
    for (int i = 0; text && i < length; i++) {
        units += text[i] > 0xFFFF ? 2 : 1;
    }
    return units;
}

static void uia_note_edit(win32_capture_ctx_t *ctx, int caret_delta,
                          bool injection_succeeded)
{
    ULONGLONG now = GetTickCount64();
    ctx->uia_last_input_ms = now;

    if (!injection_succeeded) {
        telex_reset_tracking(ctx->tctx);
        uia_invalidate_caret(ctx);
        return;
    }
    if (ctx->uia_selection_latched || !ctx->uia_caret_valid) {
        ctx->uia_invalidated_at_ms = now;
        return;
    }

    if ((caret_delta < 0 && ctx->uia_expected_caret < -caret_delta) ||
        (caret_delta > 0 && ctx->uia_expected_caret > LONG_MAX - caret_delta)) {
        uia_invalidate_caret(ctx);
        return;
    }
    ctx->uia_expected_caret += caret_delta;
}

static bool is_printable_vk(DWORD vk)
{
    return vk == VK_SPACE || (vk >= '0' && vk <= '9') ||
           (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) ||
           (vk >= VK_MULTIPLY && vk <= VK_DIVIDE) ||
           (vk >= VK_OEM_1 && vk <= VK_OEM_8) || vk == VK_OEM_102 ||
           vk == VK_DECIMAL;
}

static void show_tray_menu(HWND hwnd)
{
    HMENU menu = CreatePopupMenu();
    if (!menu) return;

    bool startup_enabled = false;
    (void)get_startup_enabled(&startup_enabled);

    const char *toggle_text = g_capture_ctx->vn_enabled
        ? "Disable Vietnamese"
        : "Enable Vietnamese";
    AppendMenuA(menu, MF_STRING, QUACK_TRAY_TOGGLE, toggle_text);
    AppendMenuA(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(menu, MF_STRING, QUACK_TRAY_STARTUP,
                startup_enabled ? "Remove from Windows startup"
                                : "Add to Windows startup");
    AppendMenuA(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(menu, MF_STRING, QUACK_TRAY_UPDATE,
                "Check for updates");
    AppendMenuA(menu, MF_STRING, QUACK_TRAY_UNINSTALL,
                "Uninstall keyboard-quack");
    AppendMenuA(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(menu, MF_STRING, QUACK_TRAY_EXIT, "Exit keyboard-quack");

    POINT point;
    GetCursorPos(&point);
    SetForegroundWindow(hwnd);
    UINT command = TrackPopupMenu(menu,
                                 TPM_RIGHTBUTTON | TPM_RETURNCMD,
                                 point.x, point.y, 0, hwnd, NULL);
    PostMessageA(hwnd, WM_NULL, 0, 0);
    DestroyMenu(menu);

    if (command == QUACK_TRAY_TOGGLE) {
        win32_capture_set_enabled(g_capture_ctx, !g_capture_ctx->vn_enabled);
    } else if (command == QUACK_TRAY_STARTUP) {
        DWORD error = set_startup_enabled(!startup_enabled);
        if (error != ERROR_SUCCESS) {
            show_startup_error(hwnd, error);
        }
    } else if (command == QUACK_TRAY_UNINSTALL) {
        uninstall_current_copy(hwnd);
    } else if (command == QUACK_TRAY_UPDATE) {
        win_update_start_check(hwnd);
    } else if (command == QUACK_TRAY_EXIT) {
        win32_capture_stop(g_capture_ctx);
    }
}

static LRESULT CALLBACK TrayWindowProc(HWND hwnd, UINT message,
                                       WPARAM wParam, LPARAM lParam)
{
    if (message == QUACK_TRAY_MESSAGE && g_capture_ctx) {
        if (lParam == WM_LBUTTONUP) {
            win32_capture_set_enabled(g_capture_ctx,
                                      !g_capture_ctx->vn_enabled);
            return 0;
        }
        if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
            show_tray_menu(hwnd);
            return 0;
        }
    }

    if (message == WM_CLOSE) {
        if (g_capture_ctx) win32_capture_stop(g_capture_ctx);
        return 0;
    }

    return DefWindowProcA(hwnd, message, wParam, lParam);
}

static int create_tray_icon(win32_capture_ctx_t *ctx)
{
    WNDCLASSEXA window_class;
    ZeroMemory(&window_class, sizeof(window_class));
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = TrayWindowProc;
    window_class.hInstance = GetModuleHandleA(NULL);
    window_class.lpszClassName = QUACK_WINDOW_CLASS;

    if (!RegisterClassExA(&window_class) &&
        GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return -1;
    }

    ctx->tray_window = CreateWindowExA(0, QUACK_WINDOW_CLASS,
                                       "keyboard-quack",
                                       WS_OVERLAPPED,
                                       0, 0, 0, 0,
                                       NULL, NULL,
                                       GetModuleHandleA(NULL), NULL);
    if (!ctx->tray_window) return -1;

    ZeroMemory(&ctx->tray_icon_data, sizeof(ctx->tray_icon_data));
    ctx->tray_icon_data.cbSize = sizeof(ctx->tray_icon_data);
    ctx->tray_icon_data.hWnd = ctx->tray_window;
    ctx->tray_icon_data.uID = 1;
    ctx->tray_icon_data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    ctx->tray_icon_data.uCallbackMessage = QUACK_TRAY_MESSAGE;
    ctx->tray_icon_data.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    lstrcpynA(ctx->tray_icon_data.szTip,
              "keyboard-quack: Vietnamese ON",
              (int)(sizeof(ctx->tray_icon_data.szTip) /
                    sizeof(ctx->tray_icon_data.szTip[0])));

    if (!Shell_NotifyIconA(NIM_ADD, &ctx->tray_icon_data)) {
        DestroyWindow(ctx->tray_window);
        ctx->tray_window = NULL;
        return -1;
    }

    ctx->tray_icon_visible = true;
    return 0;
}

static uint16_t vk_to_telex_keycode(DWORD vk)
{
    switch (vk) {
    case 'Q': return 16;
    case 'W': return 17;
    case 'E': return 18;
    case 'R': return 19;
    case 'T': return 20;
    case 'Y': return 21;
    case 'U': return 22;
    case 'I': return 23;
    case 'O': return 24;
    case 'P': return 25;
    case 'A': return 30;
    case 'S': return 31;
    case 'D': return 32;
    case 'F': return 33;
    case 'G': return 34;
    case 'H': return 35;
    case 'J': return 36;
    case 'K': return 37;
    case 'L': return 38;
    case 'Z': return 44;
    case 'X': return 45;
    case 'C': return 46;
    case 'V': return 47;
    case 'B': return 48;
    case 'N': return 49;
    case 'M': return 50;
    default: return 0;
    }
}

static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam,
                                          LPARAM lParam)
{
    if (nCode == HC_ACTION && g_capture_ctx) {
        bool clicked =
            wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN ||
            wParam == WM_MBUTTONDOWN || wParam == WM_XBUTTONDOWN ||
            wParam == WM_NCLBUTTONDOWN || wParam == WM_NCRBUTTONDOWN ||
            wParam == WM_NCMBUTTONDOWN || wParam == WM_NCXBUTTONDOWN;
        if (clicked) {
            /* A click may reposition the caret or create a selection. */
            telex_reset_tracking(g_capture_ctx->tctx);
            uia_invalidate_caret(g_capture_ctx);
        }
    }

    HHOOK hook = g_capture_ctx ? g_capture_ctx->mouse_hook : NULL;
    return CallNextHookEx(hook, nCode, wParam, lParam);
}

static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode != HC_ACTION || !g_capture_ctx) {
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    KBDLLHOOKSTRUCT *p = (KBDLLHOOKSTRUCT *)lParam;

    /* Ignore events injected by keyboard-quack or other synthetic sources with our tag */
    if ((p->flags & LLKHF_INJECTED) || (p->dwExtraInfo == (ULONG_PTR)QUACK_MAGIC_INJECT)) {
        return CallNextHookEx(g_capture_ctx->hook, nCode, wParam, lParam);
    }

    bool is_down = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
    bool is_up   = (wParam == WM_KEYUP   || wParam == WM_SYSKEYUP);
    DWORD vk = p->vkCode;

    bool ctrl_down  = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool shift_down = (GetAsyncKeyState(VK_SHIFT)   & 0x8000) != 0;
    bool alt_down   = (GetAsyncKeyState(VK_MENU)    & 0x8000) != 0;
    bool win_down   = ((GetAsyncKeyState(VK_LWIN)   & 0x8000) != 0) ||
                      ((GetAsyncKeyState(VK_RWIN)   & 0x8000) != 0);
    bool caps_on    = (GetKeyState(VK_CAPITAL)     & 0x0001) != 0;

    /* Hotkey: Ctrl + Shift toggle */
    if (vk == VK_LSHIFT || vk == VK_RSHIFT || vk == VK_LCONTROL || vk == VK_RCONTROL) {
        if (!ctrl_down && !shift_down) {
            g_capture_ctx->ctrl_shift_latched = 0;
        } else if (ctrl_down && shift_down && !g_capture_ctx->ctrl_shift_latched) {
            win32_capture_set_enabled(g_capture_ctx, !g_capture_ctx->vn_enabled);
            telex_reset_tracking(g_capture_ctx->tctx);
            uia_invalidate_caret(g_capture_ctx);
            g_capture_ctx->ctrl_shift_latched = 1;
        }
        return CallNextHookEx(g_capture_ctx->hook, nCode, wParam, lParam);
    }

    /* Hotkey: Ctrl + Space toggle */
    if (vk == VK_SPACE && ctrl_down && is_down) {
        win32_capture_set_enabled(g_capture_ctx, !g_capture_ctx->vn_enabled);
        telex_reset_tracking(g_capture_ctx->tctx);
        uia_invalidate_caret(g_capture_ctx);
        return 1; /* Swallow */
    }

    if (is_down) {
        reset_on_foreground_change(g_capture_ctx);
        uia_reconcile_before_key(g_capture_ctx);
    }

    /* When system modifiers (Ctrl/Alt/Win) are held, pass through and commit word */
    if (ctrl_down || alt_down || win_down) {
        if (is_down) {
            telex_reset_tracking(g_capture_ctx->tctx);
            uia_invalidate_caret(g_capture_ctx);
        }
        return CallNextHookEx(g_capture_ctx->hook, nCode, wParam, lParam);
    }

    /* Space key */
    if (vk == VK_SPACE) {
        if (is_down) {
            telex_commit_word(g_capture_ctx->tctx);
            uia_note_edit(g_capture_ctx, 1, true);
        }
        return CallNextHookEx(g_capture_ctx->hook, nCode, wParam, lParam);
    }

    /* Backspace key */
    if (vk == VK_BACK) {
        if (is_down) {
            telex_handle_backspace(g_capture_ctx->tctx);
            uia_note_edit(g_capture_ctx, -1, true);
        }
        return CallNextHookEx(g_capture_ctx->hook, nCode, wParam, lParam);
    }

    /* Navigation, Enter, Tab, Escape, Delete, etc. invalidate the old word. */
    if (vk == VK_RETURN || vk == VK_TAB || vk == VK_ESCAPE ||
        vk == VK_LEFT || vk == VK_RIGHT || vk == VK_UP || vk == VK_DOWN ||
        vk == VK_HOME || vk == VK_END || vk == VK_PRIOR || vk == VK_NEXT ||
        vk == VK_DELETE || vk == VK_INSERT || vk == VK_CLEAR) {
        if (is_down) {
            telex_reset_tracking(g_capture_ctx->tctx);
            uia_invalidate_caret(g_capture_ctx);
        }
        return CallNextHookEx(g_capture_ctx->hook, nCode, wParam, lParam);
    }

    /* Non-letter keys (digits, punctuation) */
    if (vk < 'A' || vk > 'Z') {
        if (is_down) {
            telex_commit_word(g_capture_ctx->tctx);
            if (is_printable_vk(vk)) {
                uia_note_edit(g_capture_ctx, 1, true);
            }
        }
        return CallNextHookEx(g_capture_ctx->hook, nCode, wParam, lParam);
    }

    /* Letter keys ('A'..'Z') */
    if (!g_capture_ctx->vn_enabled) {
        if (is_down) {
            uia_note_edit(g_capture_ctx, 1, true);
        }
        return CallNextHookEx(g_capture_ctx->hook, nCode, wParam, lParam);
    }

    if (is_down) {
        uint16_t code = vk_to_telex_keycode(vk);
        bool is_upper = (shift_down ^ caps_on) != 0;
        telex_result_t result = telex_process(g_capture_ctx->tctx, code, true, is_upper);

        switch (result.action) {
        case ACT_NONE:
            return 1; /* Swallow */

        case ACT_OUTPUT:
            if (result.output_len > 0) {
                int inject_result = win32_inject_string(g_capture_ctx->ictx,
                                                        result.output,
                                                        result.output_len);
                uia_note_edit(g_capture_ctx,
                              output_utf16_length(result.output,
                                                  result.output_len),
                              inject_result == 0);
            }
            return 1; /* Swallow original raw key */

        case ACT_BKSP_OUTPUT:
            if (result.backspace_count > 0 || result.output_len > 0) {
                int inject_result = win32_inject_bksp_retype(
                    g_capture_ctx->ictx, result.backspace_count,
                    result.output, result.output_len);
                uia_note_edit(g_capture_ctx,
                              output_utf16_length(result.output,
                                                  result.output_len) -
                                  result.backspace_count,
                              inject_result == 0);
            }
            return 1; /* Swallow original raw key */

        default:
            uia_note_edit(g_capture_ctx, 1, true);
            break;
        }
    } else if (is_up) {
        /* Swallow key up for processed letter keys to avoid phantom releases */
        return 1;
    }

    return CallNextHookEx(g_capture_ctx->hook, nCode, wParam, lParam);
}

int win32_capture_init(win32_capture_ctx_t *ctx, telex_ctx_t *tctx, win32_inject_ctx_t *ictx)
{
    ZeroMemory(ctx, sizeof(*ctx));
    ctx->tctx = tctx;
    ctx->ictx = ictx;
    ctx->thread_id = GetCurrentThreadId();
    ctx->hook = NULL;
    ctx->tray_window = NULL;
    ctx->running = 0;
    ctx->vn_enabled = 1;
    ctx->ctrl_shift_latched = 0;
    ctx->tray_icon_visible = false;

    if (win32_uia_start(&ctx->uia) < 0) {
        fprintf(stderr, "[capture] UI Automation unavailable; using keyboard-only tracking\n");
    }

    g_capture_ctx = ctx;

    /* Ensure the hook's message queue exists before another thread can stop it. */
    MSG msg;
    PeekMessageA(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    ctx->hook = SetWindowsHookExA(WH_KEYBOARD_LL, LowLevelKeyboardProc,
                                 GetModuleHandleA(NULL), 0);
    if (!ctx->hook) {
        fprintf(stderr, "[capture] Failed to install Windows keyboard hook: error %lu\n", GetLastError());
        win32_uia_stop(&ctx->uia);
        g_capture_ctx = NULL;
        return -1;
    }

    ctx->mouse_hook = SetWindowsHookExA(WH_MOUSE_LL, LowLevelMouseProc,
                                        GetModuleHandleA(NULL), 0);
    if (!ctx->mouse_hook) {
        fprintf(stderr, "[capture] Failed to install Windows mouse hook: error %lu\n", GetLastError());
        UnhookWindowsHookEx(ctx->hook);
        ctx->hook = NULL;
        win32_uia_stop(&ctx->uia);
        g_capture_ctx = NULL;
        return -1;
    }

    if (create_tray_icon(ctx) < 0) {
        fprintf(stderr, "[capture] Failed to create Windows system tray icon: error %lu\n", GetLastError());
        UnhookWindowsHookEx(ctx->hook);
        ctx->hook = NULL;
        UnhookWindowsHookEx(ctx->mouse_hook);
        ctx->mouse_hook = NULL;
        win32_uia_stop(&ctx->uia);
        g_capture_ctx = NULL;
        return -1;
    }

    fprintf(stderr, "[capture] Installed Windows Low-Level Keyboard Hook successfully\n");
    return 0;
}

void win32_capture_set_enabled(win32_capture_ctx_t *ctx, int enabled)
{
    ctx->vn_enabled = enabled;
    if (ctx->tctx) {
        telex_set_enabled(ctx->tctx, enabled);
    }
    if (ctx->tray_icon_visible) {
        lstrcpynA(ctx->tray_icon_data.szTip,
                  enabled ? "keyboard-quack: Vietnamese ON"
                          : "keyboard-quack: Vietnamese OFF",
                  (int)(sizeof(ctx->tray_icon_data.szTip) /
                        sizeof(ctx->tray_icon_data.szTip[0])));
        Shell_NotifyIconA(NIM_MODIFY, &ctx->tray_icon_data);
    }
    fprintf(stderr, "[quack] Vietnamese %s\n", enabled ? "ENABLED" : "DISABLED");
}

int win32_capture_run_loop(win32_capture_ctx_t *ctx)
{
    ctx->running = 1;
    MSG msg;
    while (ctx->running) {
        BOOL bRet = GetMessageA(&msg, NULL, 0, 0);
        if (bRet <= 0) {
            break;
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return 0;
}

void win32_capture_stop(win32_capture_ctx_t *ctx)
{
    ctx->running = 0;
    if (ctx->thread_id != 0) {
        PostThreadMessageA(ctx->thread_id, WM_QUIT, 0, 0);
    }
}

void win32_capture_cleanup(win32_capture_ctx_t *ctx)
{
    if (ctx->tray_icon_visible) {
        Shell_NotifyIconA(NIM_DELETE, &ctx->tray_icon_data);
        ctx->tray_icon_visible = false;
    }
    if (ctx->tray_window) {
        DestroyWindow(ctx->tray_window);
        ctx->tray_window = NULL;
        UnregisterClassA(QUACK_WINDOW_CLASS, GetModuleHandleA(NULL));
    }
    if (ctx->hook) {
        UnhookWindowsHookEx(ctx->hook);
        ctx->hook = NULL;
    }
    if (ctx->mouse_hook) {
        UnhookWindowsHookEx(ctx->mouse_hook);
        ctx->mouse_hook = NULL;
    }
    win32_uia_stop(&ctx->uia);
    if (g_capture_ctx == ctx) {
        g_capture_ctx = NULL;
    }
}

#endif /* _WIN32 */
