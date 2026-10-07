#ifndef WIN32_UIA_H
#define WIN32_UIA_H

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>
#include <stdbool.h>

#define WIN32_UIA_TEXT_CAP 257

typedef struct {
    bool available;
    bool has_selection;
    DWORD process_id;
    HWND element_window;
    ULONGLONG element_runtime_id;
    LONG caret_offset;
    ULONGLONG sampled_at_ms;
    bool text_before_caret_available;
    bool text_before_caret_stable;
    int text_before_caret_len;
    WCHAR text_before_caret[WIN32_UIA_TEXT_CAP];
} win32_uia_snapshot_t;

typedef struct {
    HANDLE worker;
    HANDLE stop_event;
    CRITICAL_SECTION snapshot_lock;
    bool lock_initialized;
    win32_uia_snapshot_t snapshot;
} win32_uia_ctx_t;

/* Start a background UI Automation reader. Failure is non-fatal. */
int win32_uia_start(win32_uia_ctx_t *ctx);

/* Copy the most recent caret/selection snapshot without making UIA calls. */
bool win32_uia_get_snapshot(win32_uia_ctx_t *ctx,
                            win32_uia_snapshot_t *snapshot);

void win32_uia_stop(win32_uia_ctx_t *ctx);

#endif /* _WIN32 */
#endif /* WIN32_UIA_H */
