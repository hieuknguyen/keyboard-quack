#ifndef WIN32_INJECT_H
#define WIN32_INJECT_H

#if defined(_WIN32) || defined(_WIN64)

#include <stdint.h>
#include <stdbool.h>

#define QUACK_MAGIC_INJECT 0x51554143ULL /* 'QUAC' */

typedef struct {
    int initialized;
} win32_inject_ctx_t;

/*
 * Initialize the Windows injection backend.
 */
int win32_inject_init(win32_inject_ctx_t *ctx);

/*
 * Inject a single Unicode character (UTF-16 codepoint).
 */
int win32_inject_unicode(win32_inject_ctx_t *ctx, uint32_t codepoint);

/*
 * Inject a sequence of Unicode characters.
 */
int win32_inject_string(win32_inject_ctx_t *ctx, const uint32_t *codepoints, int len);

/*
 * Send N backspaces.
 */
int win32_inject_backspace(win32_inject_ctx_t *ctx, int count);

/*
 * Send N backspaces and then type the replacement string in a single atomic batch.
 */
int win32_inject_bksp_retype(win32_inject_ctx_t *ctx, int bksp_count,
                             const uint32_t *codepoints, int len);

/*
 * Cleanup.
 */
void win32_inject_cleanup(win32_inject_ctx_t *ctx);

#endif /* _WIN32 */
#endif /* WIN32_INJECT_H */
