#ifndef UINPUT_INJECT_H
#define UINPUT_INJECT_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int uinput_fd;

    /* Dead-key layout (custom XKB "quack" layout) active -> primary path */
    int layout_active;

    /* X11 fallback (dlopen'd at runtime, no build dependency) */
    void *x11_lib;   /* dlopen handle for libX11.so.6 */
    void *xtst_lib;  /* dlopen handle for libXtst.so.6 */
    void *dpy;       /* Display* */
    int   x11_ok;    /* X11 display opened successfully */
    int   keycode;   /* spare keycode used for keysym remapping */
    int   bspc_keycode; /* X11 keycode for BackSpace */

    /* ydotool fallback (Wayland without XWayland) */
    int   use_ydotool;

    int   backend;   /* 0=none, 1=x11, 2=ydotool, 3=uinput-only, 4=deadkeys */
} inject_ctx_t;

/*
 * Initialize the injection backend.
 * Creates a uinput device + detects the best Unicode path:
 *   4 = dead-key XKB layout active (recommended, works on Wayland)
 *   1 = X11 XTest (Xorg only)
 *   3 = uinput-only (no Unicode support)
 * Returns 0 on success.
 */
int inject_init(inject_ctx_t *ctx);

/*
 * Inject a single key event (press/release) via uinput.
 */
int inject_key(inject_ctx_t *ctx, uint16_t keycode, bool pressed);

/*
 * Inject a key event with exact evdev value: 0=release, 1=press, 2=repeat.
 */
int inject_key_val(inject_ctx_t *ctx, uint16_t keycode, int val);

/*
 * Inject a single Unicode character (works for ASCII too).
 */
int inject_unicode(inject_ctx_t *ctx, uint32_t codepoint);

/*
 * Inject a sequence of Unicode characters.
 */
int inject_string(inject_ctx_t *ctx, const uint32_t *codepoints, int len);

/*
 * Send N backspaces (via uinput, or XTest if X11 backend).
 */
int inject_backspace(inject_ctx_t *ctx, int count);

/*
 * Send backspaces then a sequence of chars, strictly ordered.
 */
int inject_bksp_retype(inject_ctx_t *ctx, int bksp_count,
                       const uint32_t *codepoints, int len);

/*
 * Cleanup.
 */
void inject_cleanup(inject_ctx_t *ctx);

#endif /* UINPUT_INJECT_H */