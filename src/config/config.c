#include "config.h"
#include "../platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void config_defaults(quack_config_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->input_method = 0;       /* Telex */
    cfg->toggle_key = QUACK_TOGGLE_NONE;
    cfg->auto_start = false;
    cfg->show_tray = true;
    cfg->enable_terminal = true;
    cfg->enable_password = false;
    cfg->debug = false;
}

const char *config_get_default_path(void)
{
    static char path[CONFIG_PATH_MAX];
    char config_dir[CONFIG_PATH_MAX];
    platform_get_config_dir(config_dir, sizeof(config_dir));

#if defined(_WIN32) || defined(_WIN64)
    snprintf(path, sizeof(path), "%.*s\\config.toml",
             (int)(sizeof(path) - sizeof("\\config.toml")), config_dir);
#else
    snprintf(path, sizeof(path), "%s/config.toml", config_dir);
#endif

    return path;
}

/* Simple TOML-like parser (key = value pairs) */
int config_load(quack_config_t *cfg, const char *path)
{
    config_defaults(cfg);

    if (!path || path[0] == '\0') path = config_get_default_path();
    size_t path_len = strlen(path);
    if (path_len >= sizeof(cfg->config_path)) {
        path_len = sizeof(cfg->config_path) - 1;
    }
    memcpy(cfg->config_path, path, path_len);
    cfg->config_path[path_len] = '\0';

    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "[config] No config file found at %s, using defaults\n", path);
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        /* Skip comments and empty lines */
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\0') continue;

        /* Parse key = value */
        char key[128], val[128];
        if (sscanf(p, "%127[^=]= %127[^\n]", key, val) == 2) {
            /* Trim key */
            char *end = key + strlen(key) - 1;
            while (end > key && (*end == ' ' || *end == '\t')) *end-- = '\0';

            /* Trim value */
            char *vstart = val;
            while (*vstart == ' ' || *vstart == '\t' || *vstart == '"') vstart++;
            end = vstart + strlen(vstart) - 1;
            while (end > vstart && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '"')) *end-- = '\0';

            /* Parse values */
            if (strcmp(key, "input_method") == 0) {
                if (strcmp(vstart, "telex") == 0) cfg->input_method = 0;
                else if (strcmp(vstart, "vni") == 0) cfg->input_method = 1;
            } else if (strcmp(key, "toggle_key") == 0) {
                if (strcmp(vstart, "none") == 0) cfg->toggle_key = QUACK_TOGGLE_NONE;
                else if (strcmp(vstart, "ctrl_space") == 0) cfg->toggle_key = QUACK_TOGGLE_CTRL_SPACE;
                else if (strcmp(vstart, "ctrl_shift") == 0) cfg->toggle_key = QUACK_TOGGLE_CTRL_SHIFT;
                else if (strcmp(vstart, "win_space") == 0) cfg->toggle_key = QUACK_TOGGLE_WIN_SPACE;
                else if (strcmp(vstart, "ctrl_alt_v") == 0) cfg->toggle_key = QUACK_TOGGLE_CTRL_ALT_V;
                else if (strcmp(vstart, "ctrl_shift_v") == 0) cfg->toggle_key = QUACK_TOGGLE_CTRL_SHIFT_V;
                else if (strcmp(vstart, "alt_space") == 0) cfg->toggle_key = QUACK_TOGGLE_ALT_SPACE;
                else if (strcmp(vstart, "capslock") == 0) cfg->toggle_key = QUACK_TOGGLE_CAPSLOCK;
                else if (strcmp(vstart, "grave") == 0) cfg->toggle_key = QUACK_TOGGLE_GRAVE;
            } else if (strcmp(key, "auto_start") == 0) {
                cfg->auto_start = (strcmp(vstart, "true") == 0);
            } else if (strcmp(key, "show_tray") == 0) {
                cfg->show_tray = (strcmp(vstart, "true") == 0);
            } else if (strcmp(key, "enable_terminal") == 0) {
                cfg->enable_terminal = (strcmp(vstart, "true") == 0);
            } else if (strcmp(key, "enable_password") == 0) {
                cfg->enable_password = (strcmp(vstart, "true") == 0);
            } else if (strcmp(key, "debug") == 0) {
                cfg->debug = (strcmp(vstart, "true") == 0);
            }
        }
    }

    fclose(f);
    fprintf(stderr, "[config] Loaded from %s\n", path);
    return 0;
}

int config_save(const quack_config_t *cfg, const char *path)
{
    if (!path || path[0] == '\0') path = cfg->config_path;
    if (!path || path[0] == '\0') path = config_get_default_path();

    /* Extract directory part and ensure it exists */
    char dir[CONFIG_PATH_MAX];
    strncpy(dir, path, sizeof(dir) - 1);
    dir[sizeof(dir) - 1] = '\0';
    char *last_sep = strrchr(dir, '/');
    if (!last_sep) last_sep = strrchr(dir, '\\');
    if (last_sep) {
        *last_sep = '\0';
        platform_mkdir(dir);
    }

    FILE *f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "[config] Failed to save to %s: %s\n", path, strerror(errno));
        return -1;
    }

    fprintf(f, "# keyboard-quack configuration\n");
    fprintf(f, "# Input method: telex or vni\n");
    fprintf(f, "input_method = \"%s\"\n", cfg->input_method == 0 ? "telex" : "vni");
    fprintf(f, "\n");
    const char *toggle_key_name = "none";
    switch (cfg->toggle_key) {
    case QUACK_TOGGLE_CTRL_SPACE: toggle_key_name = "ctrl_space"; break;
    case QUACK_TOGGLE_CTRL_SHIFT: toggle_key_name = "ctrl_shift"; break;
    case QUACK_TOGGLE_WIN_SPACE: toggle_key_name = "win_space"; break;
    case QUACK_TOGGLE_CTRL_ALT_V: toggle_key_name = "ctrl_alt_v"; break;
    case QUACK_TOGGLE_CTRL_SHIFT_V: toggle_key_name = "ctrl_shift_v"; break;
    case QUACK_TOGGLE_ALT_SPACE: toggle_key_name = "alt_space"; break;
    case QUACK_TOGGLE_CAPSLOCK: toggle_key_name = "capslock"; break;
    case QUACK_TOGGLE_GRAVE: toggle_key_name = "grave"; break;
    default: break;
    }
    fprintf(f, "# Toggle shortcut: none, ctrl_space, ctrl_shift, win_space, ctrl_alt_v,\n");
    fprintf(f, "# ctrl_shift_v, alt_space, capslock, or grave\n");
    fprintf(f, "toggle_key = \"%s\"\n", toggle_key_name);
    fprintf(f, "\n");
    fprintf(f, "# Other settings\n");
    fprintf(f, "auto_start = %s\n", cfg->auto_start ? "true" : "false");
    fprintf(f, "show_tray = %s\n", cfg->show_tray ? "true" : "false");
    fprintf(f, "enable_terminal = %s\n", cfg->enable_terminal ? "true" : "false");
    fprintf(f, "enable_password = %s\n", cfg->enable_password ? "true" : "false");
    fprintf(f, "debug = %s\n", cfg->debug ? "true" : "false");

    fclose(f);
    fprintf(stderr, "[config] Saved to %s\n", path);
    return 0;
}
