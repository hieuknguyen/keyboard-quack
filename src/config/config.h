#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#define CONFIG_PATH_MAX 512

typedef enum {
    QUACK_TOGGLE_NONE = 0,
    QUACK_TOGGLE_CTRL_SPACE,
    QUACK_TOGGLE_CTRL_SHIFT,
    QUACK_TOGGLE_WIN_SPACE,
    QUACK_TOGGLE_CTRL_ALT_V,
    QUACK_TOGGLE_CTRL_SHIFT_V,
    QUACK_TOGGLE_ALT_SPACE,
    QUACK_TOGGLE_CAPSLOCK,
    QUACK_TOGGLE_GRAVE
} quack_toggle_key_t;

typedef struct {
    /* Input method: 0=telex, 1=vni */
    int input_method;

    /* Shortcut used to toggle Vietnamese input (quack_toggle_key_t). */
    int toggle_key;

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

/*
 * Set default configuration values.
 */
void config_defaults(quack_config_t *cfg);

/*
 * Get the default config file path (~/.config/keyboard-quack/config.toml)
 */
const char *config_get_default_path(void);

#endif /* CONFIG_H */
