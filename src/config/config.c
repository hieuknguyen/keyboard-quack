#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <pwd.h>
#include <unistd.h>

void config_defaults(quack_config_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->input_method = 0;       /* Telex */
    cfg->toggle_key = 0;         /* Ctrl+Space */
    cfg->auto_start = false;
    cfg->show_tray = true;
    cfg->enable_terminal = true;
    cfg->enable_password = false;
    cfg->debug = false;
}

static void ensure_dir(const char *path)
{
    char tmp[1024];
    snprintf(tmp, sizeof(tmp), "%s", path);
    char *p = strrchr(tmp, '/');
    if (p) {
        *p = '\0';
        mkdir(tmp, 0755);
    }
}

const char *config_get_default_path(void)
{
    static char path[CONFIG_PATH_MAX];
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (pw) home = pw->pw_dir;
    }
    if (!home) home = "/tmp";

    snprintf(path, sizeof(path), "%s/.config/keyboard-quack/config.toml", home);
    return path;
}

/* Simple TOML-like parser (key = value pairs) */
int config_load(quack_config_t *cfg, const char *path)
{
    config_defaults(cfg);

    if (!path) path = config_get_default_path();
    strncpy(cfg->config_path, path, CONFIG_PATH_MAX - 1);

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
            while (*vstart == ' ' || *vstart == '\t') vstart++;
            end = vstart + strlen(vstart) - 1;
            while (end > vstart && (*end == ' ' || *end == '\t' || *end == '\r')) *end-- = '\0';

            /* Parse values */
            if (strcmp(key, "input_method") == 0) {
                if (strcmp(vstart, "telex") == 0) cfg->input_method = 0;
                else if (strcmp(vstart, "vni") == 0) cfg->input_method = 1;
            } else if (strcmp(key, "toggle_key") == 0) {
                if (strcmp(vstart, "ctrl_space") == 0) cfg->toggle_key = 0;
                else if (strcmp(vstart, "capslock") == 0) cfg->toggle_key = 1;
                else if (strcmp(vstart, "grave") == 0) cfg->toggle_key = 2;
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
    if (!path) path = cfg->config_path;
    if (!path || path[0] == '\0') path = config_get_default_path();

    ensure_dir(path);

    FILE *f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "[config] Failed to save to %s: %s\n", path, strerror(errno));
        return -1;
    }

    fprintf(f, "# keyboard-quack configuration\n");
    fprintf(f, "# Input method: telex or vni\n");
    fprintf(f, "input_method = \"%s\"\n", cfg->input_method == 0 ? "telex" : "vni");
    fprintf(f, "\n");
    fprintf(f, "# Toggle key: ctrl_space, capslock, or grave\n");
    fprintf(f, "toggle_key = \"%s\"\n",
            cfg->toggle_key == 0 ? "ctrl_space" :
            cfg->toggle_key == 1 ? "capslock" : "grave");
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
