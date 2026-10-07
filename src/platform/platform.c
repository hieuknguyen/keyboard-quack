#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <direct.h>
#else
#include <unistd.h>
#include <pwd.h>
#include <sys/utsname.h>
#endif

os_type_t platform_detect_os(void)
{
#if defined(_WIN32) || defined(_WIN64)
    return OS_WINDOWS;
#elif defined(__linux__)
    return OS_LINUX;
#elif defined(__APPLE__)
    return OS_MACOS;
#else
    return OS_UNKNOWN;
#endif
}

const char *platform_get_os_name(void)
{
#if defined(_WIN32) || defined(_WIN64)
    static char win_name[128];
    OSVERSIONINFOEXA osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXA));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);

    /* Basic version retrieval */
    snprintf(win_name, sizeof(win_name), "Windows (Win32 Hook & SendInput Backend)");
    return win_name;
#elif defined(__linux__)
    static char linux_name[256];
    struct utsname u;
    if (uname(&u) == 0) {
        snprintf(linux_name, sizeof(linux_name), "Linux %s (evdev & uinput Backend)", u.release);
    } else {
        snprintf(linux_name, sizeof(linux_name), "Linux (evdev & uinput Backend)");
    }
    return linux_name;
#else
    return "Unknown Operating System";
#endif
}

void platform_get_config_dir(char *out_dir, size_t max_len)
{
#if defined(_WIN32) || defined(_WIN64)
    const char *appdata = getenv("APPDATA");
    if (appdata && appdata[0] != '\0') {
        snprintf(out_dir, max_len, "%s\\keyboard-quack", appdata);
    } else {
        const char *userprofile = getenv("USERPROFILE");
        if (userprofile && userprofile[0] != '\0') {
            snprintf(out_dir, max_len, "%s\\AppData\\Roaming\\keyboard-quack", userprofile);
        } else {
            snprintf(out_dir, max_len, ".\\config");
        }
    }
#else
    const char *xdg = getenv("XDG_CONFIG_HOME");
    if (xdg && xdg[0] != '\0') {
        snprintf(out_dir, max_len, "%s/keyboard-quack", xdg);
    } else {
        const char *home = getenv("HOME");
        if (!home) {
            struct passwd *pw = getpwuid(getuid());
            if (pw) home = pw->pw_dir;
        }
        if (!home) home = "/tmp";
        snprintf(out_dir, max_len, "%s/.config/keyboard-quack", home);
    }
#endif
}

int platform_mkdir(const char *path)
{
    char tmp[1024];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (len == 0) return 0;

    /* Remove trailing slash */
    if (tmp[len - 1] == '/' || tmp[len - 1] == '\\') {
        tmp[len - 1] = '\0';
    }

    for (p = tmp + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            char prev = *p;
            *p = '\0';
#if defined(_WIN32) || defined(_WIN64)
            _mkdir(tmp);
#else
            mkdir(tmp, 0755);
#endif
            *p = prev;
        }
    }

#if defined(_WIN32) || defined(_WIN64)
    return _mkdir(tmp);
#else
    return mkdir(tmp, 0755);
#endif
}
