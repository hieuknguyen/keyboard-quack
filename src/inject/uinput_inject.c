#define _DEFAULT_SOURCE
#include "uinput_inject.h"
#include "../engine/unicode_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dlfcn.h>
#include <linux/uinput.h>
#include <linux/input.h>
#include <sys/ioctl.h>

/* X11 function pointers (resolved via dlopen) */
typedef struct {
    void *(*p_XOpenDisplay)(const char *);
    int   (*p_XCloseDisplay)(void *);
    int   (*p_XChangeKeyboardMapping)(void *, int, int, unsigned long *, int);
    int   (*p_XFlush)(void *);
    unsigned long *(*p_XGetKeyboardMapping)(void *, int, int, int *);
    int   (*p_XFree)(void *);
    unsigned long (*p_XKeysymToKeycode)(void *, unsigned long);
    int   (*p_XTestFakeKeyEvent)(void *, int, int, unsigned long);
    int   (*p_XTestGrabControl)(void *, int);
} x11_api_t;

static x11_api_t X;

static void emit(int fd, int type, int code, int val)
{
    struct input_event ie;
    ie.type = type;
    ie.code = code;
    ie.value = val;
    ie.time.tv_sec = 0;
    ie.time.tv_usec = 0;
    (void)write(fd, &ie, sizeof(ie));
}

static void emit_syn(int fd)
{
    emit(fd, EV_SYN, SYN_REPORT, 0);
}

static int create_uinput_device(void)
{
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) return -1;

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    for (int i = 0; i < 256; i++) ioctl(fd, UI_SET_KEYBIT, i);
    ioctl(fd, UI_SET_EVBIT, EV_SYN);

    struct uinput_setup usetup;
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234;
    usetup.id.product = 0x5678;
    strncpy(usetup.name, "keyboard-quack", UINPUT_MAX_NAME_SIZE - 1);

    if (ioctl(fd, UI_DEV_SETUP, &usetup) < 0 ||
        ioctl(fd, UI_DEV_CREATE) < 0) {
        close(fd);
        return -1;
    }
    usleep(200000);
    return fd;
}

/* ------------------------------------------------------------------ */
/* Dead-key typing (custom XKB layout "quack", works on X11 + Wayland) */
/* ------------------------------------------------------------------ */

/*
 * Vietnamese character -> key sequence.
 * Composition rule (X11 Compose file): tone first, mark second, dot last,
 * then the base letter.  đ is a direct AltGr+d keysym.
 * tone/mark/dot are keycodes typed while AltGr (ISO_Level3_Shift) is held.
 */
typedef struct {
    uint32_t cp;
    uint8_t  tone; /* 0 or KEY_S/KEY_F/KEY_R/KEY_X */
    uint8_t  mark; /* 0 or KEY_O (circ), KEY_W (breve), KEY_Z (horn) */
    uint8_t  dot;  /* 0 or KEY_J */
    uint8_t  base; /* KEY_A/KEY_E/KEY_I/KEY_O/KEY_U/KEY_Y */
    uint8_t  direct; /* non-zero: AltGr+this key produces the char (đ) */
} vn_dead_key_t;

static const vn_dead_key_t vn_dead_keys[] = {
    /* a */
    { 0x00E1, KEY_S, 0, 0, KEY_A, 0 },
    { 0x00E0, KEY_F, 0, 0, KEY_A, 0 },
    { 0x1EA3, KEY_R, 0, 0, KEY_A, 0 },
    { 0x00E3, KEY_X, 0, 0, KEY_A, 0 },
    { 0x1EA1, 0,     0, KEY_J, KEY_A, 0 },
    { 0x0103, 0,     KEY_W, 0, KEY_A, 0 },
    { 0x1EAF, KEY_S, KEY_W, 0, KEY_A, 0 },
    { 0x1EB1, KEY_F, KEY_W, 0, KEY_A, 0 },
    { 0x1EB3, KEY_R, KEY_W, 0, KEY_A, 0 },
    { 0x1EB5, KEY_X, KEY_W, 0, KEY_A, 0 },
    { 0x1EB7, 0,     KEY_W, KEY_J, KEY_A, 0 },
    { 0x00E2, 0,     KEY_O, 0, KEY_A, 0 },
    { 0x1EA5, KEY_S, KEY_O, 0, KEY_A, 0 },
    { 0x1EA7, KEY_F, KEY_O, 0, KEY_A, 0 },
    { 0x1EA9, KEY_R, KEY_O, 0, KEY_A, 0 },
    { 0x1EAB, KEY_X, KEY_O, 0, KEY_A, 0 },
    { 0x1EAD, 0,     KEY_O, KEY_J, KEY_A, 0 },
    /* e */
    { 0x00E9, KEY_S, 0, 0, KEY_E, 0 },
    { 0x00E8, KEY_F, 0, 0, KEY_E, 0 },
    { 0x1EBB, KEY_R, 0, 0, KEY_E, 0 },
    { 0x1EBD, KEY_X, 0, 0, KEY_E, 0 },
    { 0x1EB9, 0,     0, KEY_J, KEY_E, 0 },
    { 0x00EA, 0,     KEY_O, 0, KEY_E, 0 },
    { 0x1EBF, KEY_S, KEY_O, 0, KEY_E, 0 },
    { 0x1EC1, KEY_F, KEY_O, 0, KEY_E, 0 },
    { 0x1EC3, KEY_R, KEY_O, 0, KEY_E, 0 },
    { 0x1EC5, KEY_X, KEY_O, 0, KEY_E, 0 },
    { 0x1EC7, 0,     KEY_O, KEY_J, KEY_E, 0 },
    /* i */
    { 0x00ED, KEY_S, 0, 0, KEY_I, 0 },
    { 0x00EC, KEY_F, 0, 0, KEY_I, 0 },
    { 0x1EC9, KEY_R, 0, 0, KEY_I, 0 },
    { 0x0129, KEY_X, 0, 0, KEY_I, 0 },
    { 0x1ECB, 0,     0, KEY_J, KEY_I, 0 },
    /* o */
    { 0x00F3, KEY_S, 0, 0, KEY_O, 0 },
    { 0x00F2, KEY_F, 0, 0, KEY_O, 0 },
    { 0x1ECF, KEY_R, 0, 0, KEY_O, 0 },
    { 0x00F5, KEY_X, 0, 0, KEY_O, 0 },
    { 0x1ECD, 0,     0, KEY_J, KEY_O, 0 },
    { 0x00F4, 0,     KEY_O, 0, KEY_O, 0 },
    { 0x1ED1, KEY_S, KEY_O, 0, KEY_O, 0 },
    { 0x1ED3, KEY_F, KEY_O, 0, KEY_O, 0 },
    { 0x1ED5, KEY_R, KEY_O, 0, KEY_O, 0 },
    { 0x1ED7, KEY_X, KEY_O, 0, KEY_O, 0 },
    { 0x1ED9, 0,     KEY_O, KEY_J, KEY_O, 0 },
    { 0x01A1, 0,     KEY_Z, 0, KEY_O, 0 },
    { 0x1EDB, KEY_S, KEY_Z, 0, KEY_O, 0 },
    { 0x1EDD, KEY_F, KEY_Z, 0, KEY_O, 0 },
    { 0x1EDF, KEY_R, KEY_Z, 0, KEY_O, 0 },
    { 0x1EE1, KEY_X, KEY_Z, 0, KEY_O, 0 },
    { 0x1EE3, 0,     KEY_Z, KEY_J, KEY_O, 0 },
    /* u */
    { 0x00FA, KEY_S, 0, 0, KEY_U, 0 },
    { 0x00F9, KEY_F, 0, 0, KEY_U, 0 },
    { 0x1EE7, KEY_R, 0, 0, KEY_U, 0 },
    { 0x0169, KEY_X, 0, 0, KEY_U, 0 },
    { 0x1EE5, 0,     0, KEY_J, KEY_U, 0 },
    { 0x01B0, 0,     KEY_Z, 0, KEY_U, 0 },
    { 0x1EE9, KEY_S, KEY_Z, 0, KEY_U, 0 },
    { 0x1EEB, KEY_F, KEY_Z, 0, KEY_U, 0 },
    { 0x1EED, KEY_R, KEY_Z, 0, KEY_U, 0 },
    { 0x1EEF, KEY_X, KEY_Z, 0, KEY_U, 0 },
    { 0x1EF1, 0,     KEY_Z, KEY_J, KEY_U, 0 },
    /* y */
    { 0x00FD, KEY_S, 0, 0, KEY_Y, 0 },
    { 0x1EF3, KEY_F, 0, 0, KEY_Y, 0 },
    { 0x1EF7, KEY_R, 0, 0, KEY_Y, 0 },
    { 0x1EF9, KEY_X, 0, 0, KEY_Y, 0 },
    { 0x1EF5, 0,     0, KEY_J, KEY_Y, 0 },
    /* đ */
    { 0x0111, 0, 0, 0, 0, KEY_D },
};

static const vn_dead_key_t *vn_lookup(uint32_t cp, bool *is_upper)
{
    uint32_t lower_cp = vn_to_lower(cp, is_upper);
    for (size_t i = 0; i < sizeof(vn_dead_keys) / sizeof(vn_dead_keys[0]); i++) {
        if (vn_dead_keys[i].cp == lower_cp) return &vn_dead_keys[i];
    }
    return NULL;
}

/* Type one Vietnamese char using dead keys + AltGr on the uinput device.
 * Sequence: AltGr down, [tone] [mark] [dot] (each pressed/released), AltGr up,
 * then the base letter (with Shift if uppercase). đ is a direct AltGr+d. */
static int deadkey_type(inject_ctx_t *ctx, uint32_t cp)
{
    bool is_upper = false;
    const vn_dead_key_t *vk = vn_lookup(cp, &is_upper);
    if (!vk) return -1;

    if (vk->direct) {
        inject_key(ctx, KEY_RIGHTALT, true);
        if (is_upper) inject_key(ctx, KEY_LEFTSHIFT, true);
        inject_key(ctx, vk->direct, true);
        inject_key(ctx, vk->direct, false);
        if (is_upper) inject_key(ctx, KEY_LEFTSHIFT, false);
        inject_key(ctx, KEY_RIGHTALT, false);
        return 0;
    }

    inject_key(ctx, KEY_RIGHTALT, true);
    if (vk->tone) { inject_key(ctx, vk->tone, true);  inject_key(ctx, vk->tone, false); }
    if (vk->mark) { inject_key(ctx, vk->mark, true);  inject_key(ctx, vk->mark, false); }
    if (vk->dot)  { inject_key(ctx, vk->dot, true);   inject_key(ctx, vk->dot, false); }
    inject_key(ctx, KEY_RIGHTALT, false);

    if (is_upper) inject_key(ctx, KEY_LEFTSHIFT, true);
    inject_key(ctx, vk->base, true);
    inject_key(ctx, vk->base, false);
    if (is_upper) inject_key(ctx, KEY_LEFTSHIFT, false);
    return 0;
}

/* Is the quack/custom XKB layout active? (gsettings on GNOME) */
static int check_sources_has_custom(const char *cmd)
{
    FILE *p = popen(cmd, "r");
    if (!p) return 0;
    char buf[512];
    if (fgets(buf, sizeof(buf), p) == NULL) {
        pclose(p);
        return 0;
    }
    pclose(p);
    return strstr(buf, "quack") != NULL || strstr(buf, "custom") != NULL;
}

static int detect_layout_active(void)
{
    /* Try with the current environment first. */
    if (check_sources_has_custom(
            "gsettings get org.gnome.desktop.input-sources sources 2>/dev/null"))
        return 1;

    /* Under sudo the environment is stripped: query the real user's session
     * bus by reading DBUS_SESSION_BUS_ADDRESS from their gnome-shell. */
    const char *sudo_user = getenv("SUDO_USER");
    if (sudo_user && sudo_user[0]) {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd),
            "ADDR=$(tr '\\0' '\\n' < /proc/$(pgrep -u %s gnome-shell | head -1)/environ 2>/dev/null | sed -n 's/^DBUS_SESSION_BUS_ADDRESS=//p'); "
            "runuser -u %s -- env DBUS_SESSION_BUS_ADDRESS=\"$ADDR\" "
            "gsettings get org.gnome.desktop.input-sources sources 2>/dev/null",
            sudo_user, sudo_user);
        if (check_sources_has_custom(cmd))
            return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* X11 XTest fallback (Xorg only)                                      */
/* ------------------------------------------------------------------ */

/* Unicode codepoint -> X11 keysym */
static unsigned long cp_to_keysym(uint32_t cp)
{
    if (cp <= 0xFF) return (unsigned long)cp;                 /* Latin-1 */
    return 0x01000000UL | (unsigned long)cp;                  /* Unicode range */
}

static int load_x11_api(void)
{
    memset(&X, 0, sizeof(X));

    void *libX11 = dlopen("libX11.so.6", RTLD_LAZY);
    if (!libX11) return -1;

    void *libXtst = dlopen("libXtst.so.6", RTLD_LAZY);
    if (!libXtst) return -1;

    X.p_XOpenDisplay         = (void *(*)(const char *))dlsym(libX11, "XOpenDisplay");
    X.p_XCloseDisplay        = (int (*)(void *))dlsym(libX11, "XCloseDisplay");
    X.p_XChangeKeyboardMapping = (int (*)(void *, int, int, unsigned long *, int))dlsym(libX11, "XChangeKeyboardMapping");
    X.p_XFlush               = (int (*)(void *))dlsym(libX11, "XFlush");
    X.p_XGetKeyboardMapping  = (unsigned long *(*)(void *, int, int, int *))dlsym(libX11, "XGetKeyboardMapping");
    X.p_XFree                = (int (*)(void *))dlsym(libX11, "XFree");
    X.p_XKeysymToKeycode     = (unsigned long (*)(void *, unsigned long))dlsym(libX11, "XKeysymToKeycode");
    X.p_XTestFakeKeyEvent    = (int (*)(void *, int, int, unsigned long))dlsym(libXtst, "XTestFakeKeyEvent");

    if (!X.p_XOpenDisplay || !X.p_XChangeKeyboardMapping || !X.p_XFlush ||
        !X.p_XGetKeyboardMapping || !X.p_XFree || !X.p_XKeysymToKeycode ||
        !X.p_XTestFakeKeyEvent) {
        return -1;
    }
    return 0;
}

static int find_spare_keycode(void *dpy)
{
    int keysyms_per_keycode = 0;
    unsigned long *map = X.p_XGetKeyboardMapping(dpy, 8, 248, &keysyms_per_keycode);
    if (!map || keysyms_per_keycode < 1) return -1;

    int found = -1;
    for (int kc = 8; kc < 8 + 248; kc++) {
        unsigned long *ks = &map[(kc - 8) * keysyms_per_keycode];
        int all_none = 1;
        for (int i = 0; i < keysyms_per_keycode; i++) {
            if (ks[i] != 0) { all_none = 0; break; }
        }
        if (all_none) { found = kc; break; }
    }
    X.p_XFree(map);
    return found;
}

/* XTest: temporarily remap a spare keycode to the target keysym,
 * press/release it, then restore. Preserves keysyms_per_keycode. */
static int x11_type_unicode(inject_ctx_t *ctx, uint32_t cp)
{
    if (!ctx->dpy || ctx->keycode <= 0) return -1;

    unsigned long keysym = cp_to_keysym(cp);

    int per = 0;
    unsigned long *orig = X.p_XGetKeyboardMapping(ctx->dpy, ctx->keycode, 1, &per);
    if (!orig || per < 1) return -1;

    unsigned long *newmap = malloc(sizeof(unsigned long) * (size_t)per);
    if (!newmap) { X.p_XFree(orig); return -1; }
    for (int i = 0; i < per; i++) newmap[i] = keysym;

    X.p_XChangeKeyboardMapping(ctx->dpy, ctx->keycode, per, newmap, 1);
    X.p_XFlush(ctx->dpy);

    X.p_XTestFakeKeyEvent(ctx->dpy, ctx->keycode, 1, 0);
    X.p_XTestFakeKeyEvent(ctx->dpy, ctx->keycode, 0, 0);
    X.p_XFlush(ctx->dpy);

    X.p_XChangeKeyboardMapping(ctx->dpy, ctx->keycode, per, orig, 1);
    X.p_XFlush(ctx->dpy);

    X.p_XFree(orig);
    free(newmap);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

int inject_init(inject_ctx_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->uinput_fd = -1;

    ctx->uinput_fd = create_uinput_device();
    if (ctx->uinput_fd < 0) {
        fprintf(stderr, "[inject] Warning: uinput device creation failed\n");
    }

    /* Dead-key layout active? -> best path (X11 + Wayland) */
    if (detect_layout_active()) {
        ctx->layout_active = 1;
        ctx->backend = 4;
        fprintf(stderr, "[inject] Backend: dead-key layout (quack) active\n");
        return 0;
    }

    /* X11 XTest fallback */
    if (load_x11_api() == 0 && getenv("DISPLAY")) {
        void *dpy = X.p_XOpenDisplay(NULL);
        if (dpy) {
            ctx->x11_lib = dlopen("libX11.so.6", RTLD_NOW);
            ctx->xtst_lib = dlopen("libXtst.so.6", RTLD_NOW);
            ctx->dpy = dpy;
            ctx->x11_ok = 1;
            ctx->keycode = find_spare_keycode(dpy);
            ctx->bspc_keycode = (int)X.p_XKeysymToKeycode(dpy, 0xFF08);
            ctx->backend = (ctx->keycode > 0) ? 1 : 3;
            fprintf(stderr, "[inject] Backend: X11+XTest\n");
            return 0;
        }
    }

    ctx->backend = 3;
    fprintf(stderr,
            "[inject] Backend: uinput-only (no Unicode).\n"
            "[inject] Install the quack layout for Vietnamese support:\n"
            "[inject]   sudo cp config/xkb/symbols/quack /usr/share/X11/xkb/symbols/\n"
            "[inject]   gsettings set org.gnome.desktop.input-sources sources \"[('xkb','quack')]\"\n");
    return 0;
}

int inject_key(inject_ctx_t *ctx, uint16_t keycode, bool pressed)
{
    if (ctx->uinput_fd < 0) return -1;
    emit(ctx->uinput_fd, EV_KEY, keycode, pressed ? 1 : 0);
    emit_syn(ctx->uinput_fd);
    return 0;
}

/* Physical keycodes (linux/input-event-codes.h), NOT alphabetical */
static int ascii_to_keycode(uint32_t cp)
{
    static const int letters[26] = {
        KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I,
        KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R,
        KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z
    };
    if (cp >= 'a' && cp <= 'z') return letters[cp - 'a'];
    if (cp >= 'A' && cp <= 'Z') return letters[cp - 'A'];
    if (cp == ' ')  return KEY_SPACE;
    if (cp == '\n') return KEY_ENTER;
    if (cp == '\t') return KEY_TAB;
    return -1;
}

int inject_unicode(inject_ctx_t *ctx, uint32_t codepoint)
{
    /* ASCII via uinput (fast, works everywhere) */
    if (codepoint < 128) {
        int kc = ascii_to_keycode(codepoint);
        if (kc >= 0) {
            bool shift = (codepoint >= 'A' && codepoint <= 'Z');
            if (shift) inject_key(ctx, KEY_LEFTSHIFT, true);
            inject_key(ctx, kc, true);
            inject_key(ctx, kc, false);
            if (shift) inject_key(ctx, KEY_LEFTSHIFT, false);
            return 0;
        }
    }

    /* Vietnamese via dead keys (works on X11 + Wayland) */
    bool dummy_upper = false;
    if (ctx->layout_active && vn_lookup(codepoint, &dummy_upper)) {
        return deadkey_type(ctx, codepoint);
    }

    /* X11 XTest fallback (Xorg only) */
    if (ctx->backend == 1) {
        return x11_type_unicode(ctx, codepoint);
    }

    /* ydotool fallback */
    if (ctx->use_ydotool) {
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "ydotool type --unicode 'U%04X' 2>/dev/null", codepoint);
        return system(cmd);
    }

    return -1;
}

int inject_string(inject_ctx_t *ctx, const uint32_t *codepoints, int len)
{
    for (int i = 0; i < len; i++) {
        if (inject_unicode(ctx, codepoints[i]) < 0)
            return -1;
    }
    return 0;
}

int inject_backspace(inject_ctx_t *ctx, int count)
{
    /* Prefer XTest so ordering with XTest-retyped chars is preserved */
    if (ctx->backend == 1 && ctx->bspc_keycode > 0) {
        for (int i = 0; i < count; i++) {
            X.p_XTestFakeKeyEvent(ctx->dpy, ctx->bspc_keycode, 1, 0);
            X.p_XTestFakeKeyEvent(ctx->dpy, ctx->bspc_keycode, 0, 0);
        }
        X.p_XFlush(ctx->dpy);
        return 0;
    }
    for (int i = 0; i < count; i++) {
        inject_key(ctx, KEY_BACKSPACE, true);
        inject_key(ctx, KEY_BACKSPACE, false);
    }
    return 0;
}

int inject_bksp_retype(inject_ctx_t *ctx, int bksp_count,
                       const uint32_t *codepoints, int len)
{
    /* On X11 backend: send everything through XTest so it's strictly ordered */
    if (ctx->backend == 1 && ctx->bspc_keycode > 0) {
        for (int i = 0; i < bksp_count; i++) {
            X.p_XTestFakeKeyEvent(ctx->dpy, ctx->bspc_keycode, 1, 0);
            X.p_XTestFakeKeyEvent(ctx->dpy, ctx->bspc_keycode, 0, 0);
        }
        for (int i = 0; i < len; i++) {
            uint32_t cp = codepoints[i];
            if (cp < 128) {
                unsigned long kc = X.p_XKeysymToKeycode(ctx->dpy, (unsigned long)cp);
                if (kc > 0) {
                    int shift = (cp >= 'A' && cp <= 'Z');
                    int skc = (int)X.p_XKeysymToKeycode(ctx->dpy, 0xFFE1); /* Shift_L */
                    if (shift) X.p_XTestFakeKeyEvent(ctx->dpy, skc, 1, 0);
                    X.p_XTestFakeKeyEvent(ctx->dpy, (int)kc, 1, 0);
                    X.p_XTestFakeKeyEvent(ctx->dpy, (int)kc, 0, 0);
                    if (shift) X.p_XTestFakeKeyEvent(ctx->dpy, skc, 0, 0);
                }
            } else {
                x11_type_unicode(ctx, cp);
            }
        }
        X.p_XFlush(ctx->dpy);
        return 0;
    }

    /* Dead-key / uinput backends: all events on the same device, ordered */
    inject_backspace(ctx, bksp_count);
    return inject_string(ctx, codepoints, len);
}

void inject_cleanup(inject_ctx_t *ctx)
{
    if (ctx->uinput_fd >= 0) {
        ioctl(ctx->uinput_fd, UI_DEV_DESTROY);
        close(ctx->uinput_fd);
        ctx->uinput_fd = -1;
    }
    if (ctx->dpy) {
        X.p_XCloseDisplay(ctx->dpy);
        ctx->dpy = NULL;
    }
    if (ctx->x11_lib) dlclose(ctx->x11_lib);
    if (ctx->xtst_lib) dlclose(ctx->xtst_lib);
}