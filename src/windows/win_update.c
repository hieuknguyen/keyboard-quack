#if defined(_WIN32) || defined(_WIN64)

#define _WIN32_WINNT 0x0601
#include "win_update.h"

#include <bcrypt.h>
#include <limits.h>
#include <shellapi.h>
#include <winhttp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "installer_common.h"

#define RELEASE_API_URL \
    L"https://api.github.com/repos/hieuknguyen/keyboard-quack/releases/latest"
#define EXPECTED_ASSET_NAME "keyboard-quack-setup.exe"
#define MAX_RELEASE_JSON (2u * 1024u * 1024u)
#define MAX_INSTALLER_BYTES (64u * 1024u * 1024u)
#define SHA256_BYTES 32

static volatile LONG g_update_running = 0;

static HWND safe_owner(HWND owner)
{
    return owner && IsWindow(owner) ? owner : NULL;
}

static void show_update_message(HWND owner, const WCHAR *message,
                                UINT flags)
{
    if (owner && !IsWindow(owner)) return;
    MessageBoxW(safe_owner(owner), message, L"keyboard-quack updater", flags);
}

static void show_update_error(HWND owner, const WCHAR *operation, DWORD error)
{
    WCHAR message[256];
    swprintf(message, sizeof(message) / sizeof(message[0]),
             L"%ls failed (Windows error %lu). Check your internet connection "
             L"and try again.", operation, (unsigned long)error);
    show_update_message(owner, message, MB_OK | MB_ICONERROR);
}

static DWORD open_https_request(HINTERNET session, const WCHAR *url,
                                HINTERNET *connection_out,
                                HINTERNET *request_out,
                                WCHAR *host_out, size_t host_capacity,
                                WCHAR *path_out, size_t path_capacity)
{
    URL_COMPONENTSW parts;
    ZeroMemory(&parts, sizeof(parts));
    parts.dwStructSize = sizeof(parts);
    parts.dwSchemeLength = (DWORD)-1;
    parts.dwHostNameLength = (DWORD)-1;
    parts.dwUrlPathLength = (DWORD)-1;
    parts.dwExtraInfoLength = (DWORD)-1;
    if (!WinHttpCrackUrl(url, 0, 0, &parts)) return GetLastError();
    if (parts.nScheme != INTERNET_SCHEME_HTTPS ||
        parts.dwHostNameLength == 0 ||
        (size_t)parts.dwHostNameLength >= host_capacity) {
        return ERROR_INVALID_NAME;
    }

    size_t path_length = (size_t)parts.dwUrlPathLength +
                         (size_t)parts.dwExtraInfoLength;
    if (path_length == 0 || path_length >= path_capacity)
        return ERROR_BUFFER_OVERFLOW;

    memcpy(host_out, parts.lpszHostName,
           (size_t)parts.dwHostNameLength * sizeof(WCHAR));
    host_out[parts.dwHostNameLength] = L'\0';
    memcpy(path_out, parts.lpszUrlPath,
           (size_t)parts.dwUrlPathLength * sizeof(WCHAR));
    if (parts.dwExtraInfoLength) {
        memcpy(path_out + parts.dwUrlPathLength, parts.lpszExtraInfo,
               (size_t)parts.dwExtraInfoLength * sizeof(WCHAR));
    }
    path_out[path_length] = L'\0';

    HINTERNET connection = WinHttpConnect(session, host_out, parts.nPort, 0);
    if (!connection) return GetLastError();
    HINTERNET request = WinHttpOpenRequest(
        connection, L"GET", path_out, NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!request) {
        DWORD error = GetLastError();
        WinHttpCloseHandle(connection);
        return error;
    }

    *connection_out = connection;
    *request_out = request;
    return ERROR_SUCCESS;
}

static DWORD receive_successful_response(HINTERNET request)
{
    if (!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        return GetLastError();
    }
    if (!WinHttpReceiveResponse(request, NULL)) return GetLastError();

    DWORD status = 0;
    DWORD status_size = sizeof(status);
    if (!WinHttpQueryHeaders(request,
                             WINHTTP_QUERY_STATUS_CODE |
                                 WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &status,
                             &status_size, WINHTTP_NO_HEADER_INDEX)) {
        return GetLastError();
    }
    if (status == HTTP_STATUS_NOT_FOUND) return ERROR_NOT_FOUND;
    if (status == HTTP_STATUS_FORBIDDEN) return ERROR_ACCESS_DENIED;
    return status == HTTP_STATUS_OK ? ERROR_SUCCESS
                                    : ERROR_WINHTTP_INVALID_SERVER_RESPONSE;
}

static DWORD read_response_memory(HINTERNET request, size_t max_bytes,
                                  BYTE **data_out, size_t *length_out)
{
    BYTE *data = NULL;
    size_t length = 0;
    DWORD error = ERROR_SUCCESS;

    for (;;) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request, &available)) {
            error = GetLastError();
            break;
        }
        if (available == 0) break;
        if ((size_t)available > max_bytes - length) {
            error = ERROR_FILE_TOO_LARGE;
            break;
        }

        BYTE *next = (BYTE *)realloc(data, length + (size_t)available + 1);
        if (!next) {
            error = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        data = next;

        DWORD received = 0;
        if (!WinHttpReadData(request, data + length, available, &received)) {
            error = GetLastError();
            break;
        }
        if (received == 0) break;
        length += received;
    }

    if (error != ERROR_SUCCESS) {
        free(data);
        return error;
    }
    if (!data) {
        data = (BYTE *)malloc(1);
        if (!data) return ERROR_NOT_ENOUGH_MEMORY;
    }
    data[length] = 0;
    *data_out = data;
    *length_out = length;
    return ERROR_SUCCESS;
}

static DWORD get_latest_release_json(HINTERNET session, BYTE **json_out,
                                     size_t *json_length_out)
{
    HINTERNET connection = NULL;
    HINTERNET request = NULL;
    WCHAR host[256];
    WCHAR path[2048];
    DWORD error = open_https_request(session, RELEASE_API_URL, &connection,
                                     &request, host, 256, path, 2048);
    if (error != ERROR_SUCCESS) return error;

    static const WCHAR headers[] =
        L"Accept: application/vnd.github+json\r\n"
        L"X-GitHub-Api-Version: 2022-11-28\r\n";
    if (!WinHttpAddRequestHeaders(request, headers, (DWORD)-1,
                                  WINHTTP_ADDREQ_FLAG_ADD)) {
        error = GetLastError();
    } else {
        error = receive_successful_response(request);
    }
    if (error == ERROR_SUCCESS) {
        error = read_response_memory(request, MAX_RELEASE_JSON, json_out,
                                     json_length_out);
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    return error;
}

static bool parse_json_string(const char *json, size_t length, size_t *offset,
                              char *out, size_t capacity)
{
    size_t i = *offset;
    if (i >= length || json[i] != '"' || capacity == 0) return false;
    i++;
    size_t written = 0;

    while (i < length) {
        unsigned char c = (unsigned char)json[i++];
        if (c == '"') {
            out[written] = '\0';
            *offset = i;
            return true;
        }
        if (c == '\\') {
            if (i >= length) return false;
            c = (unsigned char)json[i++];
            switch (c) {
            case '"': case '\\': case '/': break;
            case 'b': c = '\b'; break;
            case 'f': c = '\f'; break;
            case 'n': c = '\n'; break;
            case 'r': c = '\r'; break;
            case 't': c = '\t'; break;
            case 'u':
                if (i + 4 > length) return false;
                /* The fields consumed here (version, URL, digest) are ASCII. */
                c = '?';
                i += 4;
                break;
            default: return false;
            }
        }
        if (written + 1 >= capacity) return false;
        out[written++] = (char)c;
    }
    return false;
}

static bool json_quote_is_escaped(const char *json, size_t offset)
{
    size_t slashes = 0;
    while (offset > 0 && json[offset - 1] == '\\') {
        slashes++;
        offset--;
    }
    return (slashes & 1u) != 0;
}

static bool json_get_string(const char *json, size_t length,
                            const char *property, size_t start,
                            size_t end, char *out, size_t capacity)
{
    size_t property_length = strlen(property);
    if (end > length) end = length;

    for (size_t i = start; i + property_length + 2 <= end; i++) {
        if (json[i] != '"' || json_quote_is_escaped(json, i) ||
            memcmp(json + i + 1, property, property_length) != 0 ||
            json[i + property_length + 1] != '"') {
            continue;
        }

        size_t value = i + property_length + 2;
        while (value < end && (json[value] == ' ' || json[value] == '\r' ||
                               json[value] == '\n' || json[value] == '\t')) {
            value++;
        }
        if (value >= end || json[value++] != ':') continue;
        while (value < end && (json[value] == ' ' || json[value] == '\r' ||
                               json[value] == '\n' || json[value] == '\t')) {
            value++;
        }
        if (value >= end || json[value] != '"') continue;
        return parse_json_string(json, end, &value, out, capacity);
    }
    return false;
}

static bool find_json_object_end(const char *json, size_t length,
                                 size_t start, size_t *end_out)
{
    if (start >= length || json[start] != '{') return false;
    unsigned depth = 0;
    bool in_string = false;
    bool escaped = false;

    for (size_t i = start; i < length; i++) {
        char c = json[i];
        if (in_string) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') in_string = false;
            continue;
        }
        if (c == '"') in_string = true;
        else if (c == '{') depth++;
        else if (c == '}' && depth > 0 && --depth == 0) {
            *end_out = i + 1;
            return true;
        }
    }
    return false;
}

static bool parse_version(const char *text, unsigned long parts[3])
{
    if (*text == 'v' || *text == 'V') text++;
    for (int component = 0; component < 3; component++) {
        if (*text < '0' || *text > '9') return false;
        unsigned long value = 0;
        do {
            unsigned digit = (unsigned)(*text - '0');
            if (value > (ULONG_MAX - digit) / 10) return false;
            value = value * 10 + digit;
            text++;
        } while (*text >= '0' && *text <= '9');
        parts[component] = value;
        if (component < 2) {
            if (*text++ != '.') return false;
        }
    }
    return *text == '\0';
}

static int compare_versions(const char *left, const char *right)
{
    unsigned long a[3], b[3];
    if (!parse_version(left, a) || !parse_version(right, b)) return 0;
    for (int i = 0; i < 3; i++) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
    }
    return 0;
}

static bool valid_version(const char *version)
{
    unsigned long parts[3];
    return parse_version(version, parts);
}

static bool extract_release_fields(const BYTE *json_data, size_t json_length,
                                   char *version, size_t version_capacity,
                                   WCHAR *asset_url, size_t url_capacity,
                                   char digest[72])
{
    const char *json = (const char *)json_data;
    if (!json_get_string(json, json_length, "tag_name", 0, json_length,
                         version, version_capacity)) {
        return false;
    }

    size_t assets_start = 0;
    bool found_assets = false;
    size_t property_length = strlen("assets");
    for (size_t i = 0; i + property_length + 2 <= json_length; i++) {
        if (json[i] == '"' && !json_quote_is_escaped(json, i) &&
            memcmp(json + i + 1, "assets", property_length) == 0 &&
            json[i + property_length + 1] == '"') {
            size_t value = i + property_length + 2;
            while (value < json_length &&
                   (json[value] == ' ' || json[value] == '\r' ||
                    json[value] == '\n' || json[value] == '\t')) value++;
            if (value < json_length && json[value++] == ':') {
                while (value < json_length &&
                       (json[value] == ' ' || json[value] == '\r' ||
                        json[value] == '\n' || json[value] == '\t')) value++;
                if (value < json_length && json[value] == '[') {
                    assets_start = value + 1;
                    found_assets = true;
                    break;
                }
            }
        }
    }
    if (!found_assets) return false;

    for (size_t i = assets_start; i < json_length; i++) {
        if (json[i] == ']') break;
        if (json[i] != '{') continue;
        size_t object_end = 0;
        if (!find_json_object_end(json, json_length, i, &object_end))
            return false;

        char name[128];
        if (json_get_string(json, json_length, "name", i, object_end,
                            name, sizeof(name)) &&
            strcmp(name, EXPECTED_ASSET_NAME) == 0) {
            char url[2048];
            if (!json_get_string(json, json_length, "browser_download_url",
                                 i, object_end, url, sizeof(url)) ||
                !json_get_string(json, json_length, "digest", i, object_end,
                                 digest, 72) ||
                strncmp(digest, "sha256:", 7) != 0 ||
                strlen(digest) != 7 + SHA256_BYTES * 2) {
                return false;
            }
            int converted = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                                url, -1, asset_url,
                                                (int)url_capacity);
            return converted > 0;
        }
        i = object_end - 1;
    }
    return false;
}

static bool decode_expected_digest(const char *digest, BYTE expected[SHA256_BYTES])
{
    if (strncmp(digest, "sha256:", 7) != 0 ||
        strlen(digest) != 7 + SHA256_BYTES * 2) return false;
    for (size_t i = 0; i < SHA256_BYTES; i++) {
        char hi = digest[7 + i * 2];
        char lo = digest[8 + i * 2];
        unsigned high = hi >= '0' && hi <= '9' ? (unsigned)(hi - '0')
                      : hi >= 'a' && hi <= 'f' ? (unsigned)(hi - 'a' + 10)
                      : hi >= 'A' && hi <= 'F' ? (unsigned)(hi - 'A' + 10)
                      : 16;
        unsigned low = lo >= '0' && lo <= '9' ? (unsigned)(lo - '0')
                     : lo >= 'a' && lo <= 'f' ? (unsigned)(lo - 'a' + 10)
                     : lo >= 'A' && lo <= 'F' ? (unsigned)(lo - 'A' + 10)
                     : 16;
        if (high > 15 || low > 15) return false;
        expected[i] = (BYTE)((high << 4) | low);
    }
    return true;
}

static DWORD begin_http_request(HINTERNET session, const WCHAR *url,
                                HINTERNET *connection, HINTERNET *request)
{
    WCHAR host[256];
    WCHAR path[4096];
    DWORD error = open_https_request(session, url, connection, request,
                                     host, 256, path, 4096);
    if (error != ERROR_SUCCESS) return error;

    static const WCHAR release_prefix[] =
        L"/hieuknguyen/keyboard-quack/releases/download/";
    if (_wcsicmp(host, L"github.com") != 0 ||
        wcsncmp(path, release_prefix, wcslen(release_prefix)) != 0) {
        WinHttpCloseHandle(*request);
        WinHttpCloseHandle(*connection);
        *request = NULL;
        *connection = NULL;
        return ERROR_INVALID_NAME;
    }
    return ERROR_SUCCESS;
}

static DWORD download_and_verify(HINTERNET session, const WCHAR *url,
                                 const char *digest, const WCHAR *file_path)
{
    BYTE expected[SHA256_BYTES];
    if (!decode_expected_digest(digest, expected)) return ERROR_INVALID_DATA;

    HINTERNET connection = NULL;
    HINTERNET request = NULL;
    DWORD error = begin_http_request(session, url, &connection, &request);
    if (error != ERROR_SUCCESS) return error;
    error = receive_successful_response(request);
    if (error != ERROR_SUCCESS) goto cleanup_http;

    HANDLE file = CreateFileW(file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        error = GetLastError();
        goto cleanup_http;
    }

    BCRYPT_ALG_HANDLE algorithm = NULL;
    BCRYPT_HASH_HANDLE hash = NULL;
    BYTE *hash_object = NULL;
    DWORD hash_object_length = 0;
    DWORD result_length = 0;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&algorithm,
                                                   BCRYPT_SHA256_ALGORITHM,
                                                   NULL, 0);
    if (status >= 0) {
        status = BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
                                   (PUCHAR)&hash_object_length,
                                   sizeof(hash_object_length), &result_length,
                                   0);
    }
    if (status >= 0) {
        hash_object = (BYTE *)malloc(hash_object_length);
        if (!hash_object) status = (NTSTATUS)0xC0000017L; /* no memory */
    }
    if (status >= 0) {
        status = BCryptCreateHash(algorithm, &hash, hash_object,
                                  hash_object_length, NULL, 0, 0);
    }
    if (status < 0) {
        error = ERROR_INVALID_FUNCTION;
        goto cleanup_hash;
    }

    BYTE buffer[16 * 1024];
    size_t total = 0;
    for (;;) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request, &available)) {
            error = GetLastError();
            break;
        }
        if (available == 0) break;
        DWORD to_read = available > sizeof(buffer) ? sizeof(buffer) : available;
        DWORD received = 0;
        if (!WinHttpReadData(request, buffer, to_read, &received)) {
            error = GetLastError();
            break;
        }
        if (received == 0) break;
        if ((size_t)received > MAX_INSTALLER_BYTES - total) {
            error = ERROR_FILE_TOO_LARGE;
            break;
        }
        DWORD written = 0;
        if (!WriteFile(file, buffer, received, &written, NULL) ||
            written != received) {
            error = GetLastError();
            if (error == ERROR_SUCCESS) error = ERROR_WRITE_FAULT;
            break;
        }
        if (BCryptHashData(hash, buffer, received, 0) < 0) {
            error = ERROR_INVALID_DATA;
            break;
        }
        total += received;
    }

    if (error == ERROR_SUCCESS && total == 0) error = ERROR_INVALID_DATA;
    if (error == ERROR_SUCCESS && !FlushFileBuffers(file)) error = GetLastError();

    BYTE actual[SHA256_BYTES];
    if (error == ERROR_SUCCESS &&
        BCryptFinishHash(hash, actual, sizeof(actual), 0) < 0) {
        error = ERROR_INVALID_DATA;
    }
    if (error == ERROR_SUCCESS && memcmp(actual, expected, sizeof(actual)) != 0)
        error = ERROR_CRC;

cleanup_hash:
    if (hash) BCryptDestroyHash(hash);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    free(hash_object);
    CloseHandle(file);
    if (error != ERROR_SUCCESS) DeleteFileW(file_path);

cleanup_http:
    if (request) WinHttpCloseHandle(request);
    if (connection) WinHttpCloseHandle(connection);
    return error;
}

static bool current_copy_is_installed(void)
{
    HKEY key = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, QUACK_UNINSTALL_REGISTRY_KEY, 0,
                      KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) return false;

    WCHAR install_dir[MAX_PATH];
    DWORD type = 0;
    DWORD bytes = sizeof(install_dir);
    LONG result = RegQueryValueExW(key, L"InstallLocation", NULL, &type,
                                   (BYTE *)install_dir, &bytes);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS || type != REG_SZ || bytes < sizeof(WCHAR))
        return false;
    install_dir[MAX_PATH - 1] = L'\0';

    WCHAR module_path[MAX_PATH];
    DWORD module_length = GetModuleFileNameW(NULL, module_path, MAX_PATH);
    if (module_length == 0 || module_length >= MAX_PATH) return false;
    WCHAR *separator = wcsrchr(module_path, L'\\');
    if (!separator) return false;
    *separator = L'\0';

    size_t directory_length = wcslen(install_dir);
    while (directory_length > 3 &&
           (install_dir[directory_length - 1] == L'\\' ||
            install_dir[directory_length - 1] == L'/')) {
        install_dir[--directory_length] = L'\0';
    }
    return _wcsicmp(module_path, install_dir) == 0;
}

static DWORD make_update_file_path(WCHAR *directory, size_t directory_capacity,
                                   WCHAR *installer, size_t installer_capacity)
{
    DWORD temp_length = GetTempPathW((DWORD)directory_capacity, directory);
    if (temp_length == 0) return GetLastError();
    if (temp_length >= directory_capacity) return ERROR_BUFFER_OVERFLOW;

    size_t base_length = wcslen(directory);
    if (base_length && directory[base_length - 1] != L'\\') {
        if (base_length + 1 >= directory_capacity) return ERROR_BUFFER_OVERFLOW;
        directory[base_length++] = L'\\';
        directory[base_length] = L'\0';
    }
    int folder_length = swprintf(directory + base_length,
                                 directory_capacity - base_length,
                                 L"keyboard-quack-update");
    if (folder_length < 0 ||
        (size_t)folder_length >= directory_capacity - base_length)
        return ERROR_BUFFER_OVERFLOW;

    if (!CreateDirectoryW(directory, NULL) &&
        GetLastError() != ERROR_ALREADY_EXISTS) return GetLastError();

    int path_length = swprintf(installer, installer_capacity,
                               L"%ls\\keyboard-quack-setup.exe", directory);
    if (path_length < 0 || (size_t)path_length >= installer_capacity)
        return ERROR_BUFFER_OVERFLOW;
    return ERROR_SUCCESS;
}

static DWORD WINAPI update_worker(void *parameter)
{
    HWND owner = (HWND)parameter;
    DWORD result = ERROR_SUCCESS;
    HINTERNET session = WinHttpOpen(L"keyboard-quack updater/1.0",
                                    WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                    WINHTTP_NO_PROXY_NAME,
                                    WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) {
        show_update_error(owner, L"Opening the update connection", GetLastError());
        goto done;
    }
    WinHttpSetTimeouts(session, 5000, 5000, 10000, 30000);

    BYTE *json = NULL;
    size_t json_length = 0;
    result = get_latest_release_json(session, &json, &json_length);
    if (result != ERROR_SUCCESS) {
        if (result == ERROR_NOT_FOUND) {
            show_update_message(owner,
                                L"No public GitHub Release is available yet.",
                                MB_OK | MB_ICONINFORMATION);
        } else {
            show_update_error(owner, L"Checking for updates", result);
        }
        goto cleanup_session;
    }
    if (owner && !IsWindow(owner)) goto cleanup_session;

    char latest_version[96];
    char digest[72];
    WCHAR asset_url[2048];
    if (!extract_release_fields(json, json_length, latest_version,
                                sizeof(latest_version), asset_url,
                                sizeof(asset_url) / sizeof(asset_url[0]),
                                digest)) {
        free(json);
        show_update_message(owner,
                            L"The latest release does not contain a valid "
                            L"keyboard-quack setup asset and SHA-256 digest.",
                            MB_OK | MB_ICONERROR);
        goto cleanup_session;
    }
    free(json);
    if (owner && !IsWindow(owner)) goto cleanup_session;

    if (!valid_version(latest_version)) {
        show_update_message(owner,
                            L"The latest GitHub release has an invalid "
                            L"version tag. Expected vMAJOR.MINOR.PATCH.",
                            MB_OK | MB_ICONERROR);
        goto cleanup_session;
    }

    if (compare_versions(latest_version, QUACK_VERSION_A) <= 0) {
        WCHAR message[160];
        swprintf(message, sizeof(message) / sizeof(message[0]),
                 L"keyboard-quack is up to date (version %hs).",
                 QUACK_VERSION_A);
        show_update_message(owner, message, MB_OK | MB_ICONINFORMATION);
        goto cleanup_session;
    }

    WCHAR prompt[200];
    swprintf(prompt, sizeof(prompt) / sizeof(prompt[0]),
             L"Version %hs is available. Download and start the installer now?",
             latest_version);
    if (owner && !IsWindow(owner)) goto cleanup_session;
    if (MessageBoxW(safe_owner(owner), prompt, L"keyboard-quack update",
                    MB_YESNO | MB_ICONINFORMATION | MB_DEFBUTTON1) != IDYES) {
        goto cleanup_session;
    }

    WCHAR directory[MAX_PATH];
    WCHAR installer_path[MAX_PATH];
    result = make_update_file_path(directory, MAX_PATH, installer_path,
                                   MAX_PATH);
    if (result != ERROR_SUCCESS) {
        show_update_error(owner, L"Preparing the installer download", result);
        goto cleanup_session;
    }
    DeleteFileW(installer_path);
    result = download_and_verify(session, asset_url, digest, installer_path);
    if (result != ERROR_SUCCESS) {
        show_update_error(owner, L"Downloading or verifying the installer",
                          result);
        goto cleanup_session;
    }
    if (owner && !IsWindow(owner)) {
        DeleteFileW(installer_path);
        goto cleanup_session;
    }

    HINSTANCE launched = ShellExecuteW(safe_owner(owner), L"open",
                                       installer_path, NULL, directory,
                                       SW_SHOWNORMAL);
    if ((INT_PTR)launched <= 32) {
        show_update_error(owner, L"Starting the installer",
                          (DWORD)(INT_PTR)launched);
        DeleteFileW(installer_path);
    }

cleanup_session:
    WinHttpCloseHandle(session);
done:
    InterlockedExchange(&g_update_running, 0);
    return 0;
}

void win_update_start_check(HWND owner)
{
    if (!current_copy_is_installed()) {
        show_update_message(owner,
                            L"In-app updates are available for the installed "
                            L"version. Run keyboard-quack-setup.exe first.",
                            MB_OK | MB_ICONINFORMATION);
        return;
    }

    if (InterlockedCompareExchange(&g_update_running, 1, 0) != 0) {
        show_update_message(owner,
                            L"An update check is already running.",
                            MB_OK | MB_ICONINFORMATION);
        return;
    }

    HANDLE thread = CreateThread(NULL, 0, update_worker, owner, 0, NULL);
    if (!thread) {
        DWORD error = GetLastError();
        InterlockedExchange(&g_update_running, 0);
        show_update_error(owner, L"Starting the update check", error);
        return;
    }
    CloseHandle(thread);
}

#endif
