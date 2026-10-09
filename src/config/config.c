#include "config.h"
#include "../platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

typedef struct {
    const char *name;
    uint16_t usage;
} shortcut_key_name_t;

static const shortcut_key_name_t shortcut_key_names[] = {
    {"A", 0x04}, {"B", 0x05}, {"C", 0x06}, {"D", 0x07},
    {"E", 0x08}, {"F", 0x09}, {"G", 0x0A}, {"H", 0x0B},
    {"I", 0x0C}, {"J", 0x0D}, {"K", 0x0E}, {"L", 0x0F},
    {"M", 0x10}, {"N", 0x11}, {"O", 0x12}, {"P", 0x13},
    {"Q", 0x14}, {"R", 0x15}, {"S", 0x16}, {"T", 0x17},
    {"U", 0x18}, {"V", 0x19}, {"W", 0x1A}, {"X", 0x1B},
    {"Y", 0x1C}, {"Z", 0x1D},
    {"1", 0x1E}, {"2", 0x1F}, {"3", 0x20}, {"4", 0x21},
    {"5", 0x22}, {"6", 0x23}, {"7", 0x24}, {"8", 0x25},
    {"9", 0x26}, {"0", 0x27},
    {"Enter", 0x28}, {"Return", 0x28}, {"Escape", 0x29},
    {"Esc", 0x29}, {"Backspace", 0x2A}, {"Tab", 0x2B},
    {"Space", 0x2C}, {"Minus", 0x2D}, {"-", 0x2D},
    {"Equal", 0x2E}, {"=", 0x2E}, {"[", 0x2F}, {"BracketLeft", 0x2F},
    {"]", 0x30}, {"BracketRight", 0x30}, {"Backslash", 0x31}, {"\\", 0x31},
    {"Semicolon", 0x33}, {";", 0x33}, {"Apostrophe", 0x34}, {"'", 0x34},
    {"Grave", 0x35}, {"`", 0x35}, {"Comma", 0x36}, {",", 0x36},
    {"Period", 0x37}, {".", 0x37}, {"Slash", 0x38}, {"/", 0x38},
    {"CapsLock", 0x39}, {"Caps Lock", 0x39},
    {"F1", 0x3A}, {"F2", 0x3B}, {"F3", 0x3C}, {"F4", 0x3D},
    {"F5", 0x3E}, {"F6", 0x3F}, {"F7", 0x40}, {"F8", 0x41},
    {"F9", 0x42}, {"F10", 0x43}, {"F11", 0x44}, {"F12", 0x45},
    {"PrintScreen", 0x46}, {"ScrollLock", 0x47}, {"Pause", 0x48},
    {"Insert", 0x49}, {"Home", 0x4A}, {"PageUp", 0x4B},
    {"Delete", 0x4C}, {"End", 0x4D}, {"PageDown", 0x4E},
    {"Right", 0x4F}, {"Left", 0x50}, {"Down", 0x51}, {"Up", 0x52},
    {"NumLock", 0x53}, {"NumpadDivide", 0x54}, {"NumpadMultiply", 0x55},
    {"NumpadSubtract", 0x56}, {"NumpadAdd", 0x57},
    {"NumpadEnter", 0x58}, {"Numpad1", 0x59}, {"Numpad2", 0x5A},
    {"Numpad3", 0x5B}, {"Numpad4", 0x5C}, {"Numpad5", 0x5D},
    {"Numpad6", 0x5E}, {"Numpad7", 0x5F}, {"Numpad8", 0x60},
    {"Numpad9", 0x61}, {"Numpad0", 0x62}, {"NumpadDecimal", 0x63},
    {"NonUSBackslash", 0x64}, {"Application", 0x65},
    {"Power", 0x66}, {"NumpadEqual", 0x67},
    {"F13", 0x68}, {"F14", 0x69}, {"F15", 0x6A}, {"F16", 0x6B},
    {"F17", 0x6C}, {"F18", 0x6D}, {"F19", 0x6E}, {"F20", 0x6F},
    {"F21", 0x70}, {"F22", 0x71}, {"F23", 0x72}, {"F24", 0x73}
};

static int shortcut_name_equal(const char *left, const char *right)
{
    while (*left && *right) {
        if (tolower((unsigned char)*left) != tolower((unsigned char)*right))
            return 0;
        left++;
        right++;
    }
    return *left == '\0' && *right == '\0';
}

static uint16_t shortcut_key_from_name(const char *name)
{
    size_t count = sizeof(shortcut_key_names) / sizeof(shortcut_key_names[0]);
    for (size_t i = 0; i < count; i++) {
        if (shortcut_name_equal(name, shortcut_key_names[i].name))
            return shortcut_key_names[i].usage;
    }
    return 0;
}

static const char *shortcut_name_from_key(uint16_t key)
{
    /* Prefer Qt's portable names for keys with aliases. */
    switch (key) {
    case 0x28: return "Enter";
    case 0x29: return "Escape";
    case 0x2D: return "-";
    case 0x2E: return "=";
    case 0x2F: return "[";
    case 0x30: return "]";
    case 0x31: return "Backslash";
    case 0x33: return ";";
    case 0x34: return "'";
    case 0x35: return "`";
    case 0x36: return ",";
    case 0x37: return ".";
    case 0x38: return "/";
    default: break;
    }

    size_t count = sizeof(shortcut_key_names) / sizeof(shortcut_key_names[0]);
    for (size_t i = 0; i < count; i++) {
        if (shortcut_key_names[i].usage == key)
            return shortcut_key_names[i].name;
    }
    return NULL;
}

static int shortcut_add_modifier(uint8_t *modifiers, const char *name)
{
    if (shortcut_name_equal(name, "Ctrl") ||
        shortcut_name_equal(name, "Control")) {
        *modifiers |= QUACK_SHORTCUT_MOD_CTRL;
    } else if (shortcut_name_equal(name, "Shift")) {
        *modifiers |= QUACK_SHORTCUT_MOD_SHIFT;
    } else if (shortcut_name_equal(name, "Alt")) {
        *modifiers |= QUACK_SHORTCUT_MOD_ALT;
    } else if (shortcut_name_equal(name, "Win") ||
               shortcut_name_equal(name, "Meta") ||
               shortcut_name_equal(name, "Super")) {
        *modifiers |= QUACK_SHORTCUT_MOD_WIN;
    } else {
        return 0;
    }
    return 1;
}

bool config_set_toggle_shortcut(quack_config_t *cfg, uint16_t key,
                                uint8_t modifiers)
{
    const uint8_t all_modifiers = QUACK_SHORTCUT_MOD_CTRL |
                                  QUACK_SHORTCUT_MOD_SHIFT |
                                  QUACK_SHORTCUT_MOD_ALT |
                                  QUACK_SHORTCUT_MOD_WIN;
    if (!cfg || !shortcut_name_from_key(key) || (modifiers & ~all_modifiers))
        return false;
    /* A lone Caps Lock is supported; all other shortcuts need a modifier. */
    if (modifiers == 0 && key != 0x39)
        return false;

    cfg->toggle_key = QUACK_TOGGLE_CUSTOM;
    cfg->toggle_custom_key = key;
    cfg->toggle_custom_modifiers = modifiers;
    config_format_toggle_shortcut(cfg, cfg->toggle_shortcut,
                                  sizeof(cfg->toggle_shortcut));
    return true;
}

bool config_parse_toggle_shortcut(quack_config_t *cfg, const char *text)
{
    if (!cfg || !text) return false;

    char buffer[CONFIG_SHORTCUT_MAX];
    size_t length = strlen(text);
    if (length >= sizeof(buffer)) return false;
    memcpy(buffer, text, length + 1);

    char *begin = buffer;
    while (*begin && isspace((unsigned char)*begin)) begin++;
    char *end = begin + strlen(begin);
    while (end > begin && isspace((unsigned char)end[-1])) *--end = '\0';
    if (*begin == '\0' || shortcut_name_equal(begin, "Off") ||
        shortcut_name_equal(begin, "None")) {
        cfg->toggle_key = QUACK_TOGGLE_NONE;
        cfg->toggle_custom_key = 0;
        cfg->toggle_custom_modifiers = 0;
        cfg->toggle_shortcut[0] = '\0';
        return true;
    }

    uint8_t modifiers = 0;
    size_t trimmed_length = strlen(begin);
    if (trimmed_length >= 2 && begin[trimmed_length - 1] == '+' &&
        begin[trimmed_length - 2] == '+') {
        begin[trimmed_length - 2] = '\0';
        char *part = begin;
        while (*part) {
            char *separator = strchr(part, '+');
            if (separator) *separator = '\0';
            while (*part && isspace((unsigned char)*part)) part++;
            char *part_end = part + strlen(part);
            while (part_end > part && isspace((unsigned char)part_end[-1]))
                *--part_end = '\0';
            if (!shortcut_add_modifier(&modifiers, part)) return false;
            if (!separator) break;
            part = separator + 1;
        }
        modifiers |= QUACK_SHORTCUT_MOD_SHIFT;
        return config_set_toggle_shortcut(cfg, 0x2E, modifiers);
    }

    char *key_name = begin;
    char *token = begin;
    while (*token) {
        if (*token == '+') {
            *token = '\0';
            while (*key_name && isspace((unsigned char)*key_name)) key_name++;
            char *token_end = key_name + strlen(key_name);
            while (token_end > key_name && isspace((unsigned char)token_end[-1]))
                *--token_end = '\0';
            if (!shortcut_add_modifier(&modifiers, key_name)) return false;
            key_name = token + 1;
        }
        token++;
    }

    while (*key_name && isspace((unsigned char)*key_name)) key_name++;
    end = key_name + strlen(key_name);
    while (end > key_name && isspace((unsigned char)end[-1])) *--end = '\0';
    uint16_t key = shortcut_key_from_name(key_name);
    if (!key) return false;
    return config_set_toggle_shortcut(cfg, key, modifiers);
}

void config_format_toggle_shortcut(const quack_config_t *cfg,
                                   char *out, size_t out_size)
{
    if (!out || out_size == 0) return;
    out[0] = '\0';
    if (!cfg) return;

    uint16_t key = cfg->toggle_custom_key;
    uint8_t modifiers = cfg->toggle_custom_modifiers;
    if (cfg->toggle_key != QUACK_TOGGLE_CUSTOM) {
        switch (cfg->toggle_key) {
        case QUACK_TOGGLE_CTRL_SPACE:
            key = 0x2C; modifiers = QUACK_SHORTCUT_MOD_CTRL; break;
        case QUACK_TOGGLE_WIN_SPACE:
            key = 0x2C; modifiers = QUACK_SHORTCUT_MOD_WIN; break;
        case QUACK_TOGGLE_CTRL_ALT_V:
            key = 0x19; modifiers = QUACK_SHORTCUT_MOD_CTRL | QUACK_SHORTCUT_MOD_ALT; break;
        case QUACK_TOGGLE_CTRL_SHIFT_V:
            key = 0x19; modifiers = QUACK_SHORTCUT_MOD_CTRL | QUACK_SHORTCUT_MOD_SHIFT; break;
        case QUACK_TOGGLE_CTRL_SHIFT:
            snprintf(out, out_size, "%s", "Ctrl+Shift");
            return;
        case QUACK_TOGGLE_ALT_SPACE:
            key = 0x2C; modifiers = QUACK_SHORTCUT_MOD_ALT; break;
        case QUACK_TOGGLE_CAPSLOCK:
            key = 0x39; modifiers = 0; break;
        case QUACK_TOGGLE_GRAVE:
            key = 0x35; modifiers = 0; break;
        default:
            snprintf(out, out_size, "%s", "Off");
            return;
        }
    }

    const char *key_name = shortcut_name_from_key(key);
    if (!key_name) {
        snprintf(out, out_size, "%s", "Off");
        return;
    }
    size_t used = 0;
#define APPEND_MOD(flag, label) do { \
    if (modifiers & (flag)) { \
        int n = snprintf(out + used, out_size - used, "%s%s", \
                         used ? "+" : "", (label)); \
        if (n < 0 || (size_t)n >= out_size - used) return; \
        used += (size_t)n; \
    } \
} while (0)
    APPEND_MOD(QUACK_SHORTCUT_MOD_CTRL, "Ctrl");
    APPEND_MOD(QUACK_SHORTCUT_MOD_SHIFT, "Shift");
    APPEND_MOD(QUACK_SHORTCUT_MOD_ALT, "Alt");
    APPEND_MOD(QUACK_SHORTCUT_MOD_WIN, "Win");
#undef APPEND_MOD
    snprintf(out + used, out_size - used, "%s%s", used ? "+" : "", key_name);
}

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
                else if (strcmp(vstart, "custom") == 0) cfg->toggle_key = QUACK_TOGGLE_CUSTOM;
            } else if (strcmp(key, "toggle_shortcut") == 0) {
                (void)config_parse_toggle_shortcut(cfg, vstart);
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
    case QUACK_TOGGLE_CUSTOM: toggle_key_name = "custom"; break;
    default: break;
    }
    fprintf(f, "toggle_key = \"%s\"\n", toggle_key_name);
    if (cfg->toggle_key == QUACK_TOGGLE_CUSTOM) {
        char shortcut[CONFIG_SHORTCUT_MAX];
        config_format_toggle_shortcut(cfg, shortcut, sizeof(shortcut));
        fprintf(f, "toggle_shortcut = \"%s\"\n", shortcut);
    }
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
