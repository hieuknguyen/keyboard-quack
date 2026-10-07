#if defined(_WIN32) || defined(_WIN64)

#include "win32_inject.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

int win32_inject_init(win32_inject_ctx_t *ctx)
{
    if (ctx) ctx->initialized = 1;
    fprintf(stderr, "[inject] Backend: Windows SendInput (Native Unicode UTF-16)\n");
    return 0;
}

int win32_inject_unicode(win32_inject_ctx_t *ctx, uint32_t codepoint)
{
    (void)ctx;
    INPUT inputs[2];
    ZeroMemory(inputs, sizeof(inputs));

    /* Key Down */
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = 0;
    inputs[0].ki.wScan = (WORD)codepoint;
    inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;
    inputs[0].ki.dwExtraInfo = (ULONG_PTR)QUACK_MAGIC_INJECT;

    /* Key Up */
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 0;
    inputs[1].ki.wScan = (WORD)codepoint;
    inputs[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    inputs[1].ki.dwExtraInfo = (ULONG_PTR)QUACK_MAGIC_INJECT;

    UINT sent = SendInput(2, inputs, sizeof(INPUT));
    return (sent == 2) ? 0 : -1;
}

int win32_inject_string(win32_inject_ctx_t *ctx, const uint32_t *codepoints, int len)
{
    if (len <= 0) return 0;
    return win32_inject_bksp_retype(ctx, 0, codepoints, len);
}

int win32_inject_backspace(win32_inject_ctx_t *ctx, int count)
{
    if (count <= 0) return 0;
    return win32_inject_bksp_retype(ctx, count, NULL, 0);
}

int win32_inject_bksp_retype(win32_inject_ctx_t *ctx, int bksp_count,
                             const uint32_t *codepoints, int len)
{
    (void)ctx;
    int total = (bksp_count * 2) + (len * 2);
    if (total <= 0) return 0;

    INPUT static_inputs[256];
    INPUT *inputs = static_inputs;
    if (total > 256) {
        inputs = (INPUT *)malloc(sizeof(INPUT) * (size_t)total);
        if (!inputs) return -1;
    }
    ZeroMemory(inputs, sizeof(INPUT) * (size_t)total);

    int idx = 0;

    /* 1. Backspaces */
    for (int i = 0; i < bksp_count; i++) {
        /* Press */
        inputs[idx].type = INPUT_KEYBOARD;
        inputs[idx].ki.wVk = VK_BACK;
        inputs[idx].ki.wScan = 0;
        inputs[idx].ki.dwFlags = 0;
        inputs[idx].ki.dwExtraInfo = (ULONG_PTR)QUACK_MAGIC_INJECT;
        idx++;

        /* Release */
        inputs[idx].type = INPUT_KEYBOARD;
        inputs[idx].ki.wVk = VK_BACK;
        inputs[idx].ki.wScan = 0;
        inputs[idx].ki.dwFlags = KEYEVENTF_KEYUP;
        inputs[idx].ki.dwExtraInfo = (ULONG_PTR)QUACK_MAGIC_INJECT;
        idx++;
    }

    /* 2. Replacement Unicode Characters */
    for (int i = 0; i < len; i++) {
        uint32_t cp = codepoints[i];

        /* Press */
        inputs[idx].type = INPUT_KEYBOARD;
        inputs[idx].ki.wVk = 0;
        inputs[idx].ki.wScan = (WORD)cp;
        inputs[idx].ki.dwFlags = KEYEVENTF_UNICODE;
        inputs[idx].ki.dwExtraInfo = (ULONG_PTR)QUACK_MAGIC_INJECT;
        idx++;

        /* Release */
        inputs[idx].type = INPUT_KEYBOARD;
        inputs[idx].ki.wVk = 0;
        inputs[idx].ki.wScan = (WORD)cp;
        inputs[idx].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
        inputs[idx].ki.dwExtraInfo = (ULONG_PTR)QUACK_MAGIC_INJECT;
        idx++;
    }

    /* Atomic SendInput */
    UINT sent = SendInput((UINT)total, inputs, sizeof(INPUT));

    if (inputs != static_inputs) {
        free(inputs);
    }

    return (sent == (UINT)total) ? 0 : -1;
}

void win32_inject_cleanup(win32_inject_ctx_t *ctx)
{
    if (ctx) ctx->initialized = 0;
}

#endif /* _WIN32 */
