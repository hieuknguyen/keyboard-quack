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
        "Toggle Vietnamese: Ctrl+Space (or Ctrl+Shift)\n",
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

    /* Idle timeout check: pause > 1.5s resets unfinished composition */
    uint64_t now = get_time_ms();
    if (last_event_time_ms > 0 && (now - last_event_time_ms) > 1500) {
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
            else if (shift_held && !ctrl_shift_latched) {
                toggle_vietnamese(tctx);
                ctrl_shift_latched = 1;
            }
            inject_key(ictx, code, pressed);
        }
        return;
    }
    if (code == KC_LSHIFT || code == KC_RSHIFT) {
        if (!repeated) {
            shift_held = pressed ? 1 : 0;
            if (!pressed) ctrl_shift_latched = 0;
            else if (ctrl_held && !ctrl_shift_latched) {
                toggle_vietnamese(tctx);
                ctrl_shift_latched = 1;
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
    if (code == KC_CAPS) {
        if (pressed && !repeated) {
            caps_lock = !caps_lock;
        }
        inject_key(ictx, code, pressed);
        return;
    }

    /* Ctrl+Space toggle (only on press) */
    if (code == KC_SPACE && ctrl_held && pressed) {
        toggle_vietnamese(tctx);
        return;
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
    if (!vn_enabled) {
        inject_key_val(ictx, code, val);
        return;
    }

    bool is_upper = (shift_held ^ caps_lock) != 0;
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
    fprintf(stderr, "=== keyboard-quack v1.0.1 ===\n");
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

    if (win32_capture_init(&g_win_cap, &tctx, &g_win_inj) < 0) {
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

    fprintf(stderr, "[quack] Running on Linux. Press Ctrl+Space to toggle. Ctrl+C to exit.\n\n");

    while (running) {
        struct input_event ev;
        int dev_idx = -1;

        if (capture_read(&cap, &ev, &dev_idx) < 0) {
            if (!running) break;
            continue;
        }

        bool is_mouse = capture_is_mouse(&cap, dev_idx);
        process_event(&tctx, &inj, &ev, is_mouse);
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
