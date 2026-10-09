/*
 * keyboard-quack - Vietnamese Telex Input Method for Linux & Windows
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>

#include "platform/platform.h"
#include "engine/telex.h"
#include "config/config.h"
#include "version.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include "capture/win32_capture.h"
#include "inject/win32_inject.h"
#else
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <linux/input.h>
#include "capture/evdev_capture.h"
#include "inject/uinput_inject.h"
#endif

#if !defined(_WIN32) && !defined(_WIN64)
static volatile int running = 1;
static int vn_enabled = 1;

/* Modifier keycodes (Linux / internal standard) */
#define KC_LCTRL     29
#define KC_RCTRL     97
#define KC_LSHIFT    42
#define KC_RSHIFT    54
#define KC_LALT      56
#define KC_RALT      100
#define KC_LGUI      125
#define KC_RGUI      126
#define KC_SPACE     57
#define KC_CAPS      58
#define KC_BACKSPACE 14

static void signal_handler(int sig)
{
    (void)sig;
    running = 0;
}
#else
static win32_capture_ctx_t g_win_cap;
static win32_inject_ctx_t  g_win_inj;
#endif

static void print_usage(const char *prog)
{
    fprintf(stderr,
        "keyboard-quack - Vietnamese Telex Input Method\n"
        "Platform: %s\n\n"
        "Usage: %s [options]\n"
        "  -c, --config FILE    Config file path\n"
        "  -d, --debug          Enable debug output\n"
#if !defined(_WIN32) && !defined(_WIN64)
        "  -n, --no-grab        Don't grab keyboard (Linux test mode)\n"
#endif
        "  -h, --help           Show this help\n"
        "\n"
        "Toggle Vietnamese: configure the shortcut in keyboard-quack settings\n",
        platform_get_os_name(),
        prog);
}

#if !defined(_WIN32) && !defined(_WIN64)
#ifndef _LINUX_INPUT_H
#ifndef _INPUT_EVENT_STRUCT_DEFINED
#define _INPUT_EVENT_STRUCT_DEFINED
struct input_event {
    struct {
        long tv_sec;
        long tv_usec;
    } time;
    uint16_t type;
    uint16_t code;
    int32_t value;
};
#endif
#ifndef EV_KEY
#define EV_KEY 0x01
#endif
#ifndef EV_SYN
#define EV_SYN 0x00
#endif
#ifndef SYN_REPORT
#define SYN_REPORT 0
#endif
#endif

static int ctrl_held = 0;
static int shift_held = 0;
static int alt_held = 0;
static int gui_held = 0;
static int caps_lock = 0;
static int ctrl_shift_latched = 0;
static uint16_t ctrl_shift_suppressed_code = 0;
static bool gui_space_swallowed = false;
static bool shortcut_keydown_swallowed[256];

static uint64_t last_event_time_ms = 0;

static uint64_t get_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

static void toggle_vietnamese(telex_ctx_t *tctx)
{
    vn_enabled = !vn_enabled;
    telex_set_enabled(tctx, vn_enabled);
    fprintf(stderr, "[quack] Vietnamese %s\n",
            vn_enabled ? "ENABLED" : "DISABLED");
}

/* Check if keycode is a letter key (a-z) */
static int is_letter_key(uint16_t code)
{
    return (code >= 16 && code <= 25) ||  /* q-p */
           (code >= 30 && code <= 38) ||  /* a-l */
           (code >= 44 && code <= 50);    /* z-m */
}

static uint16_t linux_key_to_hid(uint16_t code)
{
    if (code >= KEY_F13 && code <= KEY_F24)
        return (uint16_t)(0x68 + (code - KEY_F13));
    switch (code) {
    case KEY_A: return 0x04; case KEY_B: return 0x05; case KEY_C: return 0x06;
    case KEY_D: return 0x07; case KEY_E: return 0x08; case KEY_F: return 0x09;
    case KEY_G: return 0x0A; case KEY_H: return 0x0B; case KEY_I: return 0x0C;
    case KEY_J: return 0x0D; case KEY_K: return 0x0E; case KEY_L: return 0x0F;
    case KEY_M: return 0x10; case KEY_N: return 0x11; case KEY_O: return 0x12;
    case KEY_P: return 0x13; case KEY_Q: return 0x14; case KEY_R: return 0x15;
    case KEY_S: return 0x16; case KEY_T: return 0x17; case KEY_U: return 0x18;
    case KEY_V: return 0x19; case KEY_W: return 0x1A; case KEY_X: return 0x1B;
    case KEY_Y: return 0x1C; case KEY_Z: return 0x1D;
    case KEY_1: return 0x1E; case KEY_2: return 0x1F; case KEY_3: return 0x20;
    case KEY_4: return 0x21; case KEY_5: return 0x22; case KEY_6: return 0x23;
    case KEY_7: return 0x24; case KEY_8: return 0x25; case KEY_9: return 0x26;
    case KEY_0: return 0x27; case KEY_ENTER: return 0x28; case KEY_ESC: return 0x29;
    case KEY_BACKSPACE: return 0x2A; case KEY_TAB: return 0x2B; case KEY_SPACE: return 0x2C;
    case KEY_MINUS: return 0x2D; case KEY_EQUAL: return 0x2E;
    case KEY_LEFTBRACE: return 0x2F; case KEY_RIGHTBRACE: return 0x30;
    case KEY_BACKSLASH: return 0x31; case KEY_SEMICOLON: return 0x33;
    case KEY_APOSTROPHE: return 0x34; case KEY_GRAVE: return 0x35;
    case KEY_COMMA: return 0x36; case KEY_DOT: return 0x37; case KEY_SLASH: return 0x38;
    case KEY_CAPSLOCK: return 0x39;
    case KEY_F1: return 0x3A; case KEY_F2: return 0x3B; case KEY_F3: return 0x3C;
    case KEY_F4: return 0x3D; case KEY_F5: return 0x3E; case KEY_F6: return 0x3F;
    case KEY_F7: return 0x40; case KEY_F8: return 0x41; case KEY_F9: return 0x42;
    case KEY_F10: return 0x43; case KEY_F11: return 0x44; case KEY_F12: return 0x45;
    case KEY_SYSRQ: return 0x46; case KEY_SCROLLLOCK: return 0x47; case KEY_PAUSE: return 0x48;
    case KEY_INSERT: return 0x49; case KEY_HOME: return 0x4A; case KEY_PAGEUP: return 0x4B;
    case KEY_DELETE: return 0x4C; case KEY_END: return 0x4D; case KEY_PAGEDOWN: return 0x4E;
    case KEY_RIGHT: return 0x4F; case KEY_LEFT: return 0x50; case KEY_DOWN: return 0x51;
    case KEY_UP: return 0x52; case KEY_NUMLOCK: return 0x53;
    case KEY_KPSLASH: return 0x54; case KEY_KPASTERISK: return 0x55;
    case KEY_KPMINUS: return 0x56; case KEY_KPPLUS: return 0x57;
    case KEY_KPENTER: return 0x58; case KEY_KP1: return 0x59; case KEY_KP2: return 0x5A;
    case KEY_KP3: return 0x5B; case KEY_KP4: return 0x5C; case KEY_KP5: return 0x5D;
    case KEY_KP6: return 0x5E; case KEY_KP7: return 0x5F; case KEY_KP8: return 0x60;
    case KEY_KP9: return 0x61; case KEY_KP0: return 0x62; case KEY_KPDOT: return 0x63;
    case KEY_102ND: return 0x64; case KEY_COMPOSE: return 0x65;
    case KEY_KPEQUAL: return 0x67;
    default: return 0;
    }
}

static bool custom_shortcut_matches(const quack_config_t *cfg, uint16_t code)
{
    uint8_t modifiers = 0;
    if (ctrl_held) modifiers |= QUACK_SHORTCUT_MOD_CTRL;
    if (shift_held) modifiers |= QUACK_SHORTCUT_MOD_SHIFT;
    if (alt_held) modifiers |= QUACK_SHORTCUT_MOD_ALT;
    if (gui_held) modifiers |= QUACK_SHORTCUT_MOD_WIN;
    return cfg->toggle_key == QUACK_TOGGLE_CUSTOM &&
           linux_key_to_hid(code) == cfg->toggle_custom_key &&
           modifiers == cfg->toggle_custom_modifiers;
}

/* Cursor, selection, and focus keys make the tracked word no longer a reliable
 * representation of the text immediately before the caret. */
static int is_navigation_key(uint16_t code)
{
    if ((code >= KEY_F1 && code <= KEY_F10) ||
        code == KEY_F11 || code == KEY_F12) {
        return 1;
    }

    switch (code) {
    case KEY_ESC:
    case KEY_TAB:
    case KEY_ENTER:
    case KEY_KPENTER:
    case KEY_HOME:
    case KEY_PAGEUP:
    case KEY_DELETE:
    case KEY_END:
    case KEY_PAGEDOWN:
    case KEY_INSERT:
    case KEY_LEFT:
    case KEY_RIGHT:
    case KEY_UP:
    case KEY_DOWN:
    case KEY_KP0:
    case KEY_KP1:
    case KEY_KP2:
    case KEY_KP3:
    case KEY_KP4:
    case KEY_KP5:
    case KEY_KP6:
    case KEY_KP7:
    case KEY_KP8:
    case KEY_KP9:
    case KEY_KPDOT:
        return 1;
    default:
        return 0;
    }
}

static void process_event(telex_ctx_t *tctx, inject_ctx_t *ictx,
                          const quack_config_t *cfg,
                          struct input_event *ev, bool is_mouse)
{
    /* Handle pointer/mouse clicks: clicking switches focus/moves cursor, so reset tracking */
    if (is_mouse) {
        if (ev->type == EV_KEY &&
            (ev->code == BTN_LEFT || ev->code == BTN_RIGHT ||
             ev->code == BTN_MIDDLE || ev->code == BTN_SIDE ||
             ev->code == BTN_EXTRA || ev->code == BTN_TOUCH) &&
            ev->value == 1) {
            telex_reset_tracking(tctx);
        }
        return;
    }

    if (ev->type != EV_KEY) return;

    uint16_t code = ev->code;
    int val = ev->value;  /* 0=release, 1=press, 2=repeat */
    bool pressed = (val == 1);
    bool repeated = (val == 2);

    if (code < sizeof(shortcut_keydown_swallowed)) {
        if (!pressed && !repeated && shortcut_keydown_swallowed[code]) {
            shortcut_keydown_swallowed[code] = false;
            return;
        }
        if ((pressed || repeated) && shortcut_keydown_swallowed[code]) {
            return;
        }
    }

    /* Idle timeout check: pause > 1.5s resets unfinished composition */
    uint64_t now = get_time_ms();
    if (tctx->enabled && last_event_time_ms > 0 &&
        (now - last_event_time_ms) > 1500) {
        telex_reset_tracking(tctx);
    }
    if (pressed || repeated) {
        last_event_time_ms = now;
    }

    /* Always forward modifier keys (press/release only, no repeat) */
    if (code == KC_LCTRL || code == KC_RCTRL) {
        if (!repeated) {
            ctrl_held = pressed ? 1 : 0;
            if (!pressed) ctrl_shift_latched = 0;
            else if (shift_held) {
                if (!ctrl_shift_latched) {
                    if (cfg->toggle_key == QUACK_TOGGLE_CTRL_SHIFT)
                        toggle_vietnamese(tctx);
                    else if (cfg->toggle_key != QUACK_TOGGLE_CUSTOM ||
                             (cfg->toggle_custom_modifiers &
                              (QUACK_SHORTCUT_MOD_CTRL | QUACK_SHORTCUT_MOD_SHIFT)) !=
                                 (QUACK_SHORTCUT_MOD_CTRL | QUACK_SHORTCUT_MOD_SHIFT))
                        telex_reset_tracking(tctx);
                    ctrl_shift_latched = 1;
                }
                ctrl_shift_suppressed_code = code;
                return;
            }
            if (!pressed && code == ctrl_shift_suppressed_code) {
                ctrl_shift_suppressed_code = 0;
                return;
            }
            inject_key(ictx, code, pressed);
        }
        return;
    }
    if (code == KC_LSHIFT || code == KC_RSHIFT) {
        if (!repeated) {
            shift_held = pressed ? 1 : 0;
            if (!pressed) ctrl_shift_latched = 0;
            else if (ctrl_held) {
                if (!ctrl_shift_latched) {
                    if (cfg->toggle_key == QUACK_TOGGLE_CTRL_SHIFT)
                        toggle_vietnamese(tctx);
                    else if (cfg->toggle_key != QUACK_TOGGLE_CUSTOM ||
                             (cfg->toggle_custom_modifiers &
                              (QUACK_SHORTCUT_MOD_CTRL | QUACK_SHORTCUT_MOD_SHIFT)) !=
                                 (QUACK_SHORTCUT_MOD_CTRL | QUACK_SHORTCUT_MOD_SHIFT))
                        telex_reset_tracking(tctx);
                    ctrl_shift_latched = 1;
                }
                ctrl_shift_suppressed_code = code;
                return;
            }
            if (!pressed && code == ctrl_shift_suppressed_code) {
                ctrl_shift_suppressed_code = 0;
                return;
            }
            inject_key(ictx, code, pressed);
        }
        return;
    }
    if (code == KC_LALT || code == KC_RALT) {
        if (!repeated) {
            alt_held = pressed ? 1 : 0;
            if (pressed) telex_reset_tracking(tctx);
            inject_key(ictx, code, pressed);
        }
        return;
    }
    if (code == KC_LGUI || code == KC_RGUI) {
        if (!repeated) {
            gui_held = pressed ? 1 : 0;
            if (pressed) telex_reset_tracking(tctx);
            inject_key(ictx, code, pressed);
        }
        return;
    }

    if (pressed && !repeated && custom_shortcut_matches(cfg, code)) {
        toggle_vietnamese(tctx);
        shortcut_keydown_swallowed[code] = true;
        return;
    }

    if (code == KC_CAPS) {
        if (pressed && !repeated) {
            if (cfg->toggle_key == QUACK_TOGGLE_CAPSLOCK) {
                toggle_vietnamese(tctx);
                shortcut_keydown_swallowed[code] = true;
                return;
            }
            caps_lock = !caps_lock;
        }
        inject_key(ictx, code, pressed);
        return;
    }

    /* Win+Space is reserved for the user-selected toggle; swallow it by
     * default so the desktop cannot silently switch keyboard layouts. */
    if (code == KC_SPACE && gui_space_swallowed && !pressed && !repeated) {
        gui_space_swallowed = false;
        return;
    }
    if (code == KC_SPACE && gui_held && (pressed || repeated)) {
        if (pressed && !repeated && cfg->toggle_key == QUACK_TOGGLE_WIN_SPACE) {
            toggle_vietnamese(tctx);
        } else if (pressed && !repeated) {
            telex_reset_tracking(tctx);
        }
        gui_space_swallowed = true;
        return;
    }

    if (code == KC_SPACE && pressed && !repeated) {
        if (cfg->toggle_key == QUACK_TOGGLE_CTRL_SPACE && ctrl_held) {
            toggle_vietnamese(tctx);
            shortcut_keydown_swallowed[code] = true;
            return;
        }
        if (cfg->toggle_key == QUACK_TOGGLE_ALT_SPACE && alt_held) {
            toggle_vietnamese(tctx);
            shortcut_keydown_swallowed[code] = true;
            return;
        }
    }

    /* Navigation can move the caret or selection away from the tracked text.
     * Drop composition history so later Backspace cannot restore stale text. */
    if (is_navigation_key(code)) {
        if (pressed || repeated) {
            telex_reset_tracking(tctx);
        }
        inject_key_val(ictx, code, val);
        return;
    }

    if (code == 47 && pressed && !repeated &&
        ((cfg->toggle_key == QUACK_TOGGLE_CTRL_ALT_V && ctrl_held && alt_held) ||
         (cfg->toggle_key == QUACK_TOGGLE_CTRL_SHIFT_V && ctrl_held && shift_held))) {
        toggle_vietnamese(tctx);
        shortcut_keydown_swallowed[code] = true;
        return;
    }

    if (code == 41 && pressed && !repeated &&
        cfg->toggle_key == QUACK_TOGGLE_GRAVE) {
        toggle_vietnamese(tctx);
        shortcut_keydown_swallowed[code] = true;
        return;
    }

    /* When Ctrl/Alt/GUI held, pass through everything and commit word */
    if (ctrl_held || alt_held || gui_held) {
        telex_reset_tracking(tctx);
        inject_key_val(ictx, code, val);
        return;
    }

    /* === Non-letter keys: pass through with repeat support === */
    if (!is_letter_key(code)) {
        if (code == KC_SPACE) {
            if (pressed || repeated)
                telex_commit_word(tctx);
        } else if (code == KC_BACKSPACE) {
            if (pressed || repeated) {
                telex_result_t result = telex_handle_backspace(tctx);
                if (result.action == ACT_BKSP_OUTPUT) {
                    inject_bksp_retype(ictx, result.backspace_count,
                                       result.output, result.output_len);
                } else {
                    inject_key(ictx, KC_BACKSPACE, true);
                    inject_key(ictx, KC_BACKSPACE, false);
                }
            }
            return;
        } else if (pressed || repeated) {
            telex_commit_word(tctx);
        }

        inject_key_val(ictx, code, val);
        return;
    }

    /* === Letter keys: process through Telex engine (press only) === */
    bool is_upper = (shift_held ^ caps_lock) != 0;
    if (!vn_enabled) {
        (void)telex_process(tctx, code, pressed || repeated, is_upper);
        inject_key_val(ictx, code, val);
        return;
    }

    telex_result_t result = telex_process(tctx, code, pressed || repeated, is_upper);

    switch (result.action) {
    case ACT_NONE:
        break;

    case ACT_OUTPUT:
        if (result.output_len > 0) {
            inject_string(ictx, result.output, result.output_len);
        }
        break;

    case ACT_BKSP_OUTPUT:
        if (result.backspace_count > 0 || result.output_len > 0) {
            inject_bksp_retype(ictx, result.backspace_count,
                               result.output, result.output_len);
        }
        break;

    default:
        break;
    }
}
#endif /* !Windows */

int main(int argc, char *argv[])
{
    const char *config_path = NULL;
    int debug = 0;
#if !defined(_WIN32) && !defined(_WIN64)
    int no_grab = 0;
#endif

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) {
            if (i + 1 < argc) {
                config_path = argv[++i];
            }
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
            debug = 1;
#if !defined(_WIN32) && !defined(_WIN64)
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--no-grab") == 0) {
            no_grab = 1;
#endif
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }
    fprintf(stderr, "=== keyboard-quack v%s ===\n", QUACK_VERSION_STRING);
    fprintf(stderr, "Vietnamese Telex Input Method\n");
    fprintf(stderr, "[quack] Detected OS: %s\n\n", platform_get_os_name());

    quack_config_t cfg;
    config_load(&cfg, config_path);
    if (debug) cfg.debug = true;

    telex_ctx_t tctx;
    telex_init(&tctx);

#if defined(_WIN32) || defined(_WIN64)
    if (win32_inject_init(&g_win_inj) < 0) {
        MessageBoxA(NULL, "Failed to initialize keyboard input.",
                    "keyboard-quack", MB_OK | MB_ICONERROR);
        return 1;
    }

    if (win32_capture_init(&g_win_cap, &tctx, &g_win_inj, &cfg) < 0) {
        MessageBoxA(NULL,
                    "Failed to install the keyboard hook or system tray icon.",
                    "keyboard-quack", MB_OK | MB_ICONERROR);
        win32_inject_cleanup(&g_win_inj);
        return 1;
    }

    fprintf(stderr, "[quack] Running on Windows. Use the keyboard-quack tray icon to toggle or exit.\n\n");
    win32_capture_run_loop(&g_win_cap);

    fprintf(stderr, "\n[quack] Shutting down...\n");
    win32_capture_cleanup(&g_win_cap);
    win32_inject_cleanup(&g_win_inj);
    telex_reset(&tctx);
    fprintf(stderr, "[quack] Goodbye!\n");
    return 0;
#else
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    capture_ctx_t cap;
    if (capture_init(&cap) < 0) {
        fprintf(stderr, "[quack] Failed to initialize keyboard capture\n");
        return 1;
    }

    inject_ctx_t inj;
    if (inject_init(&inj) < 0) {
        fprintf(stderr, "[quack] Failed to initialize injection\n");
        capture_cleanup(&cap);
        return 1;
    }

    if (!no_grab) {
        if (capture_grab(&cap) < 0) {
            fprintf(stderr, "[quack] Failed to grab keyboard\n");
            fprintf(stderr, "[quack] Try: sudo %s or add udev rules\n", argv[0]);
            inject_cleanup(&inj);
            capture_cleanup(&cap);
            return 1;
        }
    }

    fprintf(stderr, "[quack] Running on Linux. Use the configured shortcut or tray/config UI to toggle. Ctrl+C to exit.\n\n");

    while (running) {
        struct input_event ev;
        int dev_idx = -1;

        if (capture_read(&cap, &ev, &dev_idx) < 0) {
            if (!running) break;
            continue;
        }

        bool is_mouse = capture_is_mouse(&cap, dev_idx);
        process_event(&tctx, &inj, &cfg, &ev, is_mouse);
    }

    fprintf(stderr, "\n[quack] Shutting down...\n");
    telex_reset(&tctx);
    inject_cleanup(&inj);
    capture_cleanup(&cap);
    fprintf(stderr, "[quack] Goodbye!\n");
    return 0;
#endif
}

#if defined(_WIN32) || defined(_WIN64)
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous_instance,
                   LPSTR command_line, int show_command)
{
    (void)instance;
    (void)previous_instance;
    (void)command_line;
    (void)show_command;
    return main(__argc, __argv);
}
#endif
