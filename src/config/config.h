#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#define CONFIG_PATH_MAX 512

typedef struct {
    /* Input method: 0=telex, 1=vni */
    int input_method;

    /* Toggle key: 0=Ctrl+Space, 1=CapsLock, 2=Grave */
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
