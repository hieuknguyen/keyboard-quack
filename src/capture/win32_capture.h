#ifndef WIN32_CAPTURE_H
#define WIN32_CAPTURE_H

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>
#include <shellapi.h>
#include <stdbool.h>
#include "../engine/telex.h"
#include "../inject/win32_inject.h"
#include "win32_uia.h"
#include "../config/config.h"

typedef struct {
    HHOOK              hook;
    HHOOK              mouse_hook;
    DWORD              thread_id;
    HWND               tray_window;
    NOTIFYICONDATAA    tray_icon_data;
    telex_ctx_t       *tctx;
    win32_inject_ctx_t *ictx;
    quack_config_t    *config;
    win32_uia_ctx_t    uia;
    DWORD              uia_process_id;
    HWND               uia_element_window;
    ULONGLONG          uia_element_runtime_id;
    ULONGLONG          uia_last_sample_ms;
    ULONGLONG          uia_last_input_ms;
    ULONGLONG          uia_invalidated_at_ms;
    ULONGLONG          uia_mismatch_since_ms;
    LONG               uia_expected_caret;
    LONG               uia_mismatch_caret;
    DWORD              foreground_process_id;
    HWND               foreground_window;
    bool               uia_caret_valid;
    bool               uia_selection_latched;
    bool               foreground_identity_initialized;
    bool               backspace_keydown_swallowed;
    bool               physical_left_ctrl;
    bool               physical_right_ctrl;
    bool               physical_left_shift;
    bool               physical_right_shift;
    bool               physical_left_alt;
    bool               physical_right_alt;
    bool               physical_left_win;
    bool               physical_right_win;
    bool               letter_keydown_swallowed[26];
    bool               shortcut_keydown_swallowed[256];
    bool               suppress_modifier_keyup[256];
    volatile int       running;
    int                vn_enabled;
    int                ctrl_shift_latched;
    bool               tray_icon_visible;
} win32_capture_ctx_t;

/*
 * Initialize the Windows keyboard hook capture.
 */
int win32_capture_init(win32_capture_ctx_t *ctx, telex_ctx_t *tctx,
                       win32_inject_ctx_t *ictx, quack_config_t *config);

/*
 * Enable / Disable Vietnamese processing.
 */
void win32_capture_set_enabled(win32_capture_ctx_t *ctx, int enabled);

/*
 * Run the Windows message pump. Blocks until win32_capture_stop is called.
 */
int win32_capture_run_loop(win32_capture_ctx_t *ctx);

/*
 * Stop the message pump.
 */
void win32_capture_stop(win32_capture_ctx_t *ctx);

/*
 * Unhook and cleanup.
 */
void win32_capture_cleanup(win32_capture_ctx_t *ctx);

#endif /* _WIN32 */
#endif /* WIN32_CAPTURE_H */
