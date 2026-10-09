#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CONFIG_PATH_MAX 512
#define CONFIG_SHORTCUT_MAX 64

#define QUACK_SHORTCUT_MOD_CTRL  0x01u
#define QUACK_SHORTCUT_MOD_SHIFT 0x02u
#define QUACK_SHORTCUT_MOD_ALT   0x04u
#define QUACK_SHORTCUT_MOD_WIN   0x08u

typedef enum {
    QUACK_TOGGLE_NONE = 0,
    QUACK_TOGGLE_CTRL_SPACE,
    QUACK_TOGGLE_CTRL_SHIFT,
    QUACK_TOGGLE_WIN_SPACE,
    QUACK_TOGGLE_CTRL_ALT_V,
    QUACK_TOGGLE_CTRL_SHIFT_V,
    QUACK_TOGGLE_ALT_SPACE,
    QUACK_TOGGLE_CAPSLOCK,
    QUACK_TOGGLE_GRAVE,
    QUACK_TOGGLE_CUSTOM
} quack_toggle_key_t;

typedef struct {
    /* Input method: 0=telex, 1=vni */
    int input_method;

    /* Toggle shortcut mode; custom shortcuts store a USB HID key and modifiers. */
    int toggle_key;
    uint16_t toggle_custom_key; /* USB HID keyboard usage */
    uint8_t toggle_custom_modifiers;
    char toggle_shortcut[CONFIG_SHORTCUT_MAX];

    /* Auto-start */
    bool auto_start;

    /* Show tray icon */
    bool show_tray;

    /* Enable in terminal */
    bool enable_terminal;

    /* Enable in password fields */
    bool enable_password;

    /* Debug mode */
    bool debug;

    /* Config file path */
    char config_path[CONFIG_PATH_MAX];
} quack_config_t;

/*
 * Load configuration from file.
 * If file doesn't exist, use defaults.
 */
int config_load(quack_config_t *cfg, const char *path);

/*
 * Save configuration to file.
 */
int config_save(const quack_config_t *cfg, const char *path);

/* Set, parse, and format a user-defined shortcut such as Ctrl+Alt+V. */
bool config_set_toggle_shortcut(quack_config_t *cfg, uint16_t key,
                                uint8_t modifiers);
bool config_parse_toggle_shortcut(quack_config_t *cfg, const char *text);
void config_format_toggle_shortcut(const quack_config_t *cfg,
                                   char *out, size_t out_size);

/*
 * Set default configuration values.
 */
void config_defaults(quack_config_t *cfg);

/*
 * Get the default config file path (~/.config/keyboard-quack/config.toml)
 */
const char *config_get_default_path(void);

#endif /* CONFIG_H */
