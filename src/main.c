/*
 * keyboard-quack - Vietnamese Telex Input Method for Linux
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
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
static int ctrl_shift_latched = 0;

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

static void process_event(telex_ctx_t *tctx, inject_ctx_t *ictx,
                          struct input_event *ev)
{
    if (ev->type != EV_KEY) return;

    uint16_t code = ev->code;
    int val = ev->value;  /* 0=release, 1=press, 2=repeat */
    bool pressed = (val == 1);
    bool repeated = (val == 2);

    /* Always forward modifier keys (press/release only, no repeat) */
    if (code == KC_LCTRL || code == KC_RCTRL) {
        if (!repeated) {
            ctrl_held = pressed ? 1 : 0;
            if (!pressed) ctrl_shift_latched = 0;
            else if (shift_held && !ctrl_shift_latched) {
                toggle_vietnamese(tctx);
                ctrl_shift_latched = 1;
            }
            if (!pressed && !ctrl_held && !alt_held && !gui_held) {
                /* Ctrl released - nothing special */
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
            inject_key(ictx, code, pressed);
        }
        return;
    }
    if (code == KC_LGUI || code == KC_RGUI) {
        if (!repeated) {
            gui_held = pressed ? 1 : 0;
            inject_key(ictx, code, pressed);
        }
        return;
    }

    /* Ctrl+Space toggle (only on press) */
    if (code == KC_SPACE && ctrl_held && pressed) {
        toggle_vietnamese(tctx);
        return;
    }

    /* When Ctrl/Alt/GUI held, pass through everything */
    if (ctrl_held || alt_held || gui_held) {
        telex_reset_tracking(tctx);
        inject_key(ictx, code, pressed);
        return;
    }

    /* === Non-letter keys: pass through with repeat support === */
    if (!is_letter_key(code)) {
        if (code == KC_SPACE && pressed) {
            telex_commit_boundary(tctx);
        } else if (code == KC_BACKSPACE) {
            if (pressed || repeated) {
                if (tctx->boundary_saved) {
                    telex_reset_tracking(tctx);
                    telex_restore_boundary(tctx);
                } else {
                    /* Keep the composition buffer in lockstep with the
                     * character that the physical Backspace removes.  The
                     * previous undo-based approach restored a stale token
                     * after a redraw (tone/shape), causing the next key to
                     * resurrect or duplicate text. */
                    if (tctx->word_len > 0)
                        tctx->word_len--;
                    if (tctx->rendered_len > 0)
                        tctx->rendered_len--;
                    tctx->undo_valid = false;
                    if (tctx->word_len == 0) {
                        /* A held Backspace can generate many repeats.  Once
                         * the composition is empty, discard every pending
                         * snapshot so a later key can never resurrect the
                         * deleted word. */
                        tctx->rendered_len = 0;
                        tctx->saved_word_len = 0;
                        tctx->boundary_saved = false;
                        tctx->shape_cancelled = false;
                    }
                }
            }
        } else {
            telex_reset_tracking(tctx);
        }
        if (pressed || repeated) {
            inject_key(ictx, code, true);
        } else {
            inject_key(ictx, code, false);
        }
        return;
    }

    /* === Letter keys: process through Telex engine (press only) === */
    if (!vn_enabled) {
        inject_key(ictx, code, pressed);
        return;
    }

    telex_result_t result = telex_process(tctx, code, pressed);

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
        int dev_idx;

        if (capture_read(&cap, &ev, &dev_idx) < 0) {
            if (!running) break;
            continue;
        }

        process_event(&tctx, &inj, &ev);
    }

    fprintf(stderr, "\n[quack] Shutting down...\n");
    telex_reset(&tctx);
    inject_cleanup(&inj);
    capture_cleanup(&cap);
    fprintf(stderr, "[quack] Goodbye!\n");
    return 0;
}
