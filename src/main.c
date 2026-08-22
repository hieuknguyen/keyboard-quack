/*
 * keyboard-quack - Vietnamese Telex Input Method for Linux
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <linux/input.h>

#include "engine/telex.h"
#include "capture/evdev_capture.h"
#include "inject/uinput_inject.h"
#include "config/config.h"

static volatile int running = 1;
static int vn_enabled = 1;

/* Modifier keycodes */
#define KC_LCTRL  29
#define KC_RCTRL  97
#define KC_LSHIFT 42
#define KC_RSHIFT 54
#define KC_LALT   56
#define KC_RALT   100
#define KC_LGUI   125
#define KC_RGUI   126
#define KC_SPACE  57
#define KC_CAPS   58
#define KC_BACKSPACE 14

static void signal_handler(int sig)
{
    (void)sig;
    running = 0;
}

static void print_usage(const char *prog)
{
    fprintf(stderr,
        "keyboard-quack - Vietnamese Telex Input Method\n\n"
        "Usage: %s [options]\n"
        "  -c, --config FILE    Config file path\n"
        "  -d, --debug          Enable debug output\n"
        "  -n, --no-grab        Don't grab keyboard (test mode)\n"
        "  -h, --help           Show this help\n"
        "\n"
        "Toggle Vietnamese: Ctrl+Space\n",
        prog);
}

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

/* Check if keycode is a navigation, cursor, or window control key */
static bool is_cursor_or_nav_key(uint16_t code)
{
    switch (code) {
    case 1:   /* KEY_ESC */
    case 15:  /* KEY_TAB */
    case 28:  /* KEY_ENTER */
    case 96:  /* KEY_KPENTER */
    case 102: /* KEY_HOME */
    case 103: /* KEY_UP */
    case 104: /* KEY_PAGEUP */
    case 105: /* KEY_LEFT */
    case 106: /* KEY_RIGHT */
    case 107: /* KEY_END */
    case 108: /* KEY_DOWN */
    case 109: /* KEY_PAGEDOWN */
    case 110: /* KEY_INSERT */
    case 111: /* KEY_DELETE */
        return true;
    default:
        if (code >= 59 && code <= 68) return true; /* F1 - F10 */
        if (code == 87 || code == 88) return true; /* F11, F12 */
        return false;
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

    /* When Ctrl/Alt/GUI held, pass through everything and commit word */
    if (ctrl_held || alt_held || gui_held) {
        telex_reset_tracking(tctx);
        inject_key_val(ictx, code, val);
        return;
    }

    /* Navigation / Cursor movement keys (Enter, Esc, Tab, Arrows, Home, End, Delete, F-keys) */
    if (is_cursor_or_nav_key(code)) {
        if (pressed || repeated) {
            telex_reset_tracking(tctx);
        }
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
                telex_handle_backspace(tctx);
                inject_key(ictx, KC_BACKSPACE, true);
                inject_key(ictx, KC_BACKSPACE, false);
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

int main(int argc, char *argv[])
{
    const char *config_path = NULL;
    int debug = 0;
    int no_grab = 0;

    static struct option long_opts[] = {
        {"config",   required_argument, NULL, 'c'},
        {"debug",    no_argument,       NULL, 'd'},
        {"no-grab",  no_argument,       NULL, 'n'},
        {"help",     no_argument,       NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "c:dnvh", long_opts, NULL)) != -1) {
        switch (opt) {
        case 'c': config_path = optarg; break;
        case 'd': debug = 1; break;
        case 'n': no_grab = 1; break;
        case 'h': print_usage(argv[0]); return 0;
        default:  print_usage(argv[0]); return 1;
        }
    }

    fprintf(stderr, "=== keyboard-quack v1.0.0 ===\n");
    fprintf(stderr, "Vietnamese Telex Input Method for Linux\n\n");

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    quack_config_t cfg;
    config_load(&cfg, config_path);
    if (debug) cfg.debug = true;

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

    telex_ctx_t tctx;
    telex_init(&tctx);

    if (!no_grab) {
        if (capture_grab(&cap) < 0) {
            fprintf(stderr, "[quack] Failed to grab keyboard\n");
            fprintf(stderr, "[quack] Try: sudo %s or add udev rules\n", argv[0]);
            inject_cleanup(&inj);
            capture_cleanup(&cap);
            return 1;
        }
    }

    fprintf(stderr, "[quack] Running. Ctrl+Space to toggle. Ctrl+C to exit.\n\n");

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
}
