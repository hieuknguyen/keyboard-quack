#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef enum {
    OS_UNKNOWN = 0,
    OS_LINUX,
    OS_WINDOWS,
    OS_MACOS
} os_type_t;

/*
 * Detect the current operating system.
 * Returns OS_WINDOWS, OS_LINUX, etc.
 */
os_type_t platform_detect_os(void);

/*
 * Get a human-readable description of the current OS and version.
 */
const char *platform_get_os_name(void);

/*
 * Get the default directory for keyboard-quack config and data.
 * Windows: %APPDATA%\keyboard-quack
 * Linux/macOS: ~/.config/keyboard-quack
 */
void platform_get_config_dir(char *out_dir, size_t max_len);

/*
 * Ensure a directory exists (creates parent directories if needed).
 */
int platform_mkdir(const char *path);

#endif /* PLATFORM_H */
