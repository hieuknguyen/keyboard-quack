#define COBJMACROS
#include <windows.h>
#include <objbase.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "installer_common.h"

static bool append_component(WCHAR *out, size_t capacity,
                             const WCHAR *base, const WCHAR *component)
{
    int length = swprintf(out, capacity, L"%ls\\%ls", base, component);
    return length >= 0 && (size_t)length < capacity;
}

static DWORD create_directory_tree(const WCHAR *path)
{
    int result = SHCreateDirectoryExW(NULL, path, NULL);
    if (result == ERROR_SUCCESS || result == ERROR_ALREADY_EXISTS ||
        result == ERROR_FILE_EXISTS) return ERROR_SUCCESS;
    return (DWORD)result;
}

static DWORD write_resource_to_file(WORD resource_id, const WCHAR *path)
{
    HRSRC resource = FindResourceW(GetModuleHandleW(NULL),
                                   MAKEINTRESOURCEW(resource_id),
                                   MAKEINTRESOURCEW(10));
    if (!resource) return GetLastError();

    HGLOBAL loaded = LoadResource(GetModuleHandleW(NULL), resource);
    DWORD size = SizeofResource(GetModuleHandleW(NULL), resource);
    const void *data = loaded ? LockResource(loaded) : NULL;
    if (!data || size == 0) return ERROR_RESOURCE_DATA_NOT_FOUND;

    HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return GetLastError();

    DWORD written = 0;
    BOOL write_ok = WriteFile(file, data, size, &written, NULL);
    DWORD error = write_ok && written == size ? ERROR_SUCCESS : GetLastError();
    if (write_ok && written != size && error == ERROR_SUCCESS)
        error = ERROR_WRITE_FAULT;
    if (error == ERROR_SUCCESS && !FlushFileBuffers(file))
        error = GetLastError();
    CloseHandle(file);

    if (error != ERROR_SUCCESS) DeleteFileW(path);
    return error;
}

static DWORD install_payload_file(WORD resource_id, const WCHAR *install_dir,
                                  const WCHAR *file_name)
{
    WCHAR destination[MAX_PATH];
    WCHAR temporary[MAX_PATH];
    if (!append_component(destination, MAX_PATH, install_dir, file_name))
        return ERROR_BUFFER_OVERFLOW;

    WCHAR temporary_name[MAX_PATH];
    if (swprintf(temporary_name, MAX_PATH, L"%ls.new", file_name) < 0 ||
        !append_component(temporary, MAX_PATH, install_dir, temporary_name)) {
        return ERROR_BUFFER_OVERFLOW;
    }

    DWORD error = write_resource_to_file(resource_id, temporary);
    if (error != ERROR_SUCCESS) return error;

    if (!MoveFileExW(temporary, destination,
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
        error = GetLastError();
        DeleteFileW(temporary);
        return error;
    }
    return ERROR_SUCCESS;
}

static DWORD create_shortcut(const WCHAR *shortcut_path,
                             const WCHAR *target_path,
                             const WCHAR *working_directory,
                             const WCHAR *description)
{
    IShellLinkW *shell_link = NULL;
    IPersistFile *persist_file = NULL;
    HRESULT result = CoCreateInstance(&CLSID_ShellLink, NULL,
                                      CLSCTX_INPROC_SERVER, &IID_IShellLinkW,
                                      (void **)&shell_link);
    if (FAILED(result)) return HRESULT_CODE(result);

    result = IShellLinkW_SetPath(shell_link, target_path);
    if (SUCCEEDED(result))
        result = IShellLinkW_SetWorkingDirectory(shell_link, working_directory);
    if (SUCCEEDED(result))
        result = IShellLinkW_SetDescription(shell_link, description);
    if (SUCCEEDED(result))
        result = IShellLinkW_QueryInterface(shell_link, &IID_IPersistFile,
                                            (void **)&persist_file);
    if (SUCCEEDED(result))
        result = IPersistFile_Save(persist_file, shortcut_path, TRUE);

    if (persist_file) IPersistFile_Release(persist_file);
    IShellLinkW_Release(shell_link);
    return FAILED(result) ? HRESULT_CODE(result) : ERROR_SUCCESS;
}

static DWORD create_start_menu_shortcuts(const WCHAR *install_dir,
                                        const WCHAR *app_path,
                                        const WCHAR *uninstaller_path)
{
    WCHAR programs[MAX_PATH];
    HRESULT result = SHGetFolderPathW(NULL,
                                      CSIDL_PROGRAMS | CSIDL_FLAG_CREATE,
                                      NULL, SHGFP_TYPE_CURRENT, programs);
    if (FAILED(result)) return HRESULT_CODE(result);

    WCHAR menu_folder[MAX_PATH];
    if (!append_component(menu_folder, MAX_PATH, programs,
                          QUACK_INSTALLED_APP_NAME)) {
        return ERROR_BUFFER_OVERFLOW;
    }
    DWORD directory_error = create_directory_tree(menu_folder);
    if (directory_error != ERROR_SUCCESS) return directory_error;

    WCHAR app_link[MAX_PATH];
    WCHAR uninstall_link[MAX_PATH];
    if (!append_component(app_link, MAX_PATH, menu_folder,
                          L"keyboard-quack.lnk") ||
        !append_component(uninstall_link, MAX_PATH, menu_folder,
                          L"Uninstall keyboard-quack.lnk")) {
        return ERROR_BUFFER_OVERFLOW;
    }

    HRESULT com_result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(com_result) && com_result != RPC_E_CHANGED_MODE)
        return HRESULT_CODE(com_result);
    bool initialized_com = SUCCEEDED(com_result);
    DWORD error = create_shortcut(app_link, app_path, install_dir,
                                  L"Vietnamese Telex input method");
    if (error == ERROR_SUCCESS) {
        error = create_shortcut(uninstall_link, uninstaller_path, install_dir,
                                L"Uninstall keyboard-quack");
    }
    if (initialized_com) CoUninitialize();

    if (error != ERROR_SUCCESS) {
        DeleteFileW(app_link);
        DeleteFileW(uninstall_link);
        RemoveDirectoryW(menu_folder);
    }
    return error;
}

static DWORD set_registry_string(HKEY key, const WCHAR *name,
                                 const WCHAR *value)
{
    DWORD bytes = (DWORD)((wcslen(value) + 1) * sizeof(WCHAR));
    return (DWORD)RegSetValueExW(key, name, 0, REG_SZ,
                                 (const BYTE *)value, bytes);
}

static DWORD set_registry_dword(HKEY key, const WCHAR *name, DWORD value)
{
    return (DWORD)RegSetValueExW(key, name, 0, REG_DWORD,
                                 (const BYTE *)&value, sizeof(value));
}

static DWORD register_uninstaller(const WCHAR *install_dir,
                                  const WCHAR *app_path,
                                  const WCHAR *uninstaller_path)
{
    HKEY key = NULL;
    DWORD disposition = 0;
    LONG result = RegCreateKeyExW(HKEY_CURRENT_USER,
                                  QUACK_UNINSTALL_REGISTRY_KEY, 0, NULL, 0,
                                  KEY_SET_VALUE, NULL, &key, &disposition);
    if (result != ERROR_SUCCESS) return (DWORD)result;

    WCHAR quoted_uninstaller[MAX_PATH + 8];
    WCHAR quiet_uninstaller[MAX_PATH + 24];
    WCHAR display_icon[MAX_PATH + 8];
    swprintf(quoted_uninstaller, MAX_PATH + 8, L"\"%ls\"", uninstaller_path);
    swprintf(quiet_uninstaller, MAX_PATH + 24, L"\"%ls\" /quiet",
             uninstaller_path);
    swprintf(display_icon, MAX_PATH + 8, L"%ls", app_path);

    DWORD error = set_registry_string(key, L"DisplayName",
                                      L"keyboard-quack");
    if (error == ERROR_SUCCESS)
        error = set_registry_string(key, L"DisplayVersion", QUACK_VERSION_W);
    if (error == ERROR_SUCCESS)
        error = set_registry_string(key, L"Publisher", L"keyboard-quack");
    if (error == ERROR_SUCCESS)
        error = set_registry_string(key, L"InstallLocation", install_dir);
    if (error == ERROR_SUCCESS)
        error = set_registry_string(key, L"DisplayIcon", display_icon);
    if (error == ERROR_SUCCESS)
        error = set_registry_string(key, L"UninstallString",
                                    quoted_uninstaller);
    if (error == ERROR_SUCCESS)
        error = set_registry_string(key, L"QuietUninstallString",
                                    quiet_uninstaller);
    if (error == ERROR_SUCCESS)
        error = set_registry_dword(key, L"NoModify", 1);
    if (error == ERROR_SUCCESS)
        error = set_registry_dword(key, L"NoRepair", 1);

    RegCloseKey(key);
    return error;
}

static DWORD update_existing_startup(const WCHAR *app_path)
{
    HKEY key = NULL;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER,
                                QUACK_STARTUP_REGISTRY_KEY, 0,
                                KEY_QUERY_VALUE | KEY_SET_VALUE, &key);
    if (result == ERROR_FILE_NOT_FOUND) return ERROR_SUCCESS;
    if (result != ERROR_SUCCESS) return (DWORD)result;

    DWORD type = 0;
    result = RegQueryValueExW(key, QUACK_STARTUP_REGISTRY_VALUE, NULL,
                              &type, NULL, NULL);
    if (result == ERROR_FILE_NOT_FOUND) {
        RegCloseKey(key);
        return ERROR_SUCCESS;
    }
    if (result != ERROR_SUCCESS) {
        RegCloseKey(key);
        return (DWORD)result;
    }

    WCHAR command[MAX_PATH + 8];
    swprintf(command, MAX_PATH + 8, L"\"%ls\"", app_path);
    DWORD bytes = (DWORD)((wcslen(command) + 1) * sizeof(WCHAR));
    result = RegSetValueExW(key, QUACK_STARTUP_REGISTRY_VALUE, 0, REG_SZ,
                            (const BYTE *)command, bytes);
    RegCloseKey(key);
    return (DWORD)result;
}

static DWORD install_for_current_user(WCHAR *install_dir,
                                      WCHAR *app_path,
                                      WCHAR *uninstaller_path,
                                      bool *existing_install)
{
    WCHAR local_app_data[MAX_PATH];
    DWORD root_length = GetEnvironmentVariableW(L"LOCALAPPDATA", local_app_data,
                                                MAX_PATH);
    if (root_length == 0 || root_length >= MAX_PATH) return ERROR_PATH_NOT_FOUND;

    WCHAR programs[MAX_PATH];
    if (!append_component(programs, MAX_PATH, local_app_data, L"Programs") ||
        !append_component(install_dir, MAX_PATH, programs,
                          QUACK_INSTALLED_APP_NAME)) {
        return ERROR_BUFFER_OVERFLOW;
    }
    DWORD attributes = GetFileAttributesW(install_dir);
    if (existing_install) {
        *existing_install = attributes != INVALID_FILE_ATTRIBUTES &&
                            (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }
    DWORD directory_error = create_directory_tree(install_dir);
    if (directory_error != ERROR_SUCCESS) return directory_error;

    if (!append_component(app_path, MAX_PATH, install_dir,
                          QUACK_INSTALLED_APP_EXE) ||
        !append_component(uninstaller_path, MAX_PATH, install_dir,
                          QUACK_INSTALLED_UNINSTALLER_EXE)) {
        return ERROR_BUFFER_OVERFLOW;
    }

    DWORD error = install_payload_file(IDR_QUACK_INSTALLED_EXE, install_dir,
                                       QUACK_INSTALLED_APP_EXE);
    if (error != ERROR_SUCCESS) return error;
    error = install_payload_file(IDR_QUACK_UNINSTALLER_EXE, install_dir,
                                 QUACK_INSTALLED_UNINSTALLER_EXE);
    if (error != ERROR_SUCCESS) return error;

    error = create_start_menu_shortcuts(install_dir, app_path,
                                        uninstaller_path);
    if (error != ERROR_SUCCESS) return error;
    error = register_uninstaller(install_dir, app_path, uninstaller_path);
    if (error != ERROR_SUCCESS) return error;
    return update_existing_startup(app_path);
}

static void remove_installer_shortcuts(void)
{
    WCHAR programs[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS | CSIDL_FLAG_CREATE,
                                NULL, SHGFP_TYPE_CURRENT, programs))) {
        return;
    }

    WCHAR folder[MAX_PATH];
    WCHAR shortcut[MAX_PATH];
    if (!append_component(folder, MAX_PATH, programs,
                          QUACK_INSTALLED_APP_NAME)) return;
    if (append_component(shortcut, MAX_PATH, folder, L"keyboard-quack.lnk"))
        DeleteFileW(shortcut);
    if (append_component(shortcut, MAX_PATH, folder,
                         L"Uninstall keyboard-quack.lnk")) DeleteFileW(shortcut);
    RemoveDirectoryW(folder);
}

static void remove_matching_uninstall_registration(const WCHAR *install_dir)
{
    HKEY key = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, QUACK_UNINSTALL_REGISTRY_KEY, 0,
                      KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) return;

    WCHAR registered_dir[MAX_PATH];
    DWORD type = 0;
    DWORD bytes = sizeof(registered_dir);
    LONG result = RegQueryValueExW(key, L"InstallLocation", NULL, &type,
                                   (BYTE *)registered_dir, &bytes);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS || type != REG_SZ || bytes < sizeof(WCHAR))
        return;
    registered_dir[MAX_PATH - 1] = L'\0';
    if (_wcsicmp(registered_dir, install_dir) == 0)
        RegDeleteKeyW(HKEY_CURRENT_USER, QUACK_UNINSTALL_REGISTRY_KEY);
}

static void rollback_install(const WCHAR *install_dir)
{
    WCHAR path[MAX_PATH];
    if (append_component(path, MAX_PATH, install_dir,
                         QUACK_INSTALLED_APP_EXE)) DeleteFileW(path);
    if (append_component(path, MAX_PATH, install_dir,
                         QUACK_INSTALLED_UNINSTALLER_EXE)) DeleteFileW(path);
    if (append_component(path, MAX_PATH, install_dir,
                         L"quack.exe.new")) DeleteFileW(path);
    if (append_component(path, MAX_PATH, install_dir,
                         L"uninstall.exe.new")) DeleteFileW(path);
    remove_installer_shortcuts();
    remove_matching_uninstall_registration(install_dir);
    RemoveDirectoryW(install_dir);
}

static DWORD close_running_application(void)
{
    HWND app_window = FindWindowW(QUACK_APP_WINDOW_CLASS,
                                  QUACK_APP_WINDOW_TITLE);
    if (!app_window) return ERROR_SUCCESS;

    DWORD process_id = 0;
    GetWindowThreadProcessId(app_window, &process_id);
    if (!process_id || process_id == GetCurrentProcessId())
        return ERROR_SUCCESS;

    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, process_id);
    PostMessageW(app_window, WM_CLOSE, 0, 0);
    if (!process) {
        for (int i = 0; i < 100 && IsWindow(app_window); i++) Sleep(100);
        return IsWindow(app_window) ? ERROR_TIMEOUT : ERROR_SUCCESS;
    }

    DWORD wait_result = WaitForSingleObject(process, 10000);
    CloseHandle(process);
    return wait_result == WAIT_OBJECT_0 ? ERROR_SUCCESS : ERROR_TIMEOUT;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous_instance,
                   LPSTR command_line, int show_command)
{
    (void)instance;
    (void)previous_instance;
    (void)command_line;
    (void)show_command;

    int answer = MessageBoxW(
        NULL,
        L"Install keyboard-quack for the current Windows user? This creates "
        L"Start Menu shortcuts and an uninstall entry. Existing settings and "
        L"startup preference will be preserved.",
        L"Install keyboard-quack", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);
    if (answer != IDYES) return 0;

    DWORD error = close_running_application();
    if (error != ERROR_SUCCESS) {
        MessageBoxW(NULL,
                    L"Could not close the running keyboard-quack process. "
                    L"Close it manually and run setup again.",
                    L"Install keyboard-quack", MB_OK | MB_ICONERROR);
        return 1;
    }

    WCHAR install_dir[MAX_PATH] = {0};
    WCHAR app_path[MAX_PATH] = {0};
    WCHAR uninstaller_path[MAX_PATH] = {0};
    bool existing_install = false;
    error = install_for_current_user(install_dir, app_path,
                                     uninstaller_path, &existing_install);
    if (error != ERROR_SUCCESS) {
        if (!existing_install && install_dir[0] != L'\0')
            rollback_install(install_dir);
        WCHAR message[192];
        swprintf(message, sizeof(message) / sizeof(message[0]),
                 L"Installation failed (Windows error %lu). Close keyboard-quack "
                 L"if it is running and try again.", (unsigned long)error);
        MessageBoxW(NULL, message, L"Install keyboard-quack",
                    MB_OK | MB_ICONERROR);
        return 1;
    }

    answer = MessageBoxW(NULL,
                         L"keyboard-quack was installed for this user. Run it now?",
                         L"Installation complete",
                         MB_YESNO | MB_ICONINFORMATION | MB_DEFBUTTON1);
    if (answer == IDYES) {
        HINSTANCE launched = ShellExecuteW(NULL, L"open", app_path, NULL,
                                           install_dir, SW_SHOWNORMAL);
        if ((INT_PTR)launched <= 32) {
            MessageBoxW(NULL, L"The app was installed but could not be started.",
                        L"keyboard-quack", MB_OK | MB_ICONWARNING);
        }
    }
    return 0;
}
