#ifndef TELEX_H
#define TELEX_H

#include <stdint.h>
#include <stdbool.h>

#define TELEX_MAX_OUTPUT 64
#define TELEX_MAX_WORD   TELEX_MAX_OUTPUT
#define TELEX_MAX_SUFFIX 16

/* Actions the engine tells the caller to perform */
typedef enum {
    ACT_NONE,        /* Do nothing (swallow) */
    ACT_OUTPUT,      /* Output characters in output_buf */
    ACT_BKSP_OUTPUT, /* Send N backspaces, then output (inject_bksp_retype) */
} telex_action_t;

typedef struct {
    telex_action_t action;
    uint32_t output[TELEX_MAX_OUTPUT];
    int      output_len;
    int      backspace_count;
} telex_result_t;

/* Vowel types */
typedef enum {
    VH_NONE = 0,
    VH_A, VH_ACR, VH_ACI,
    VH_E, VH_ECI,
    VH_O, VH_OCI, VH_OHR,
    VH_U, VH_UHR,
    VH_I,
    VH_Y,
} vn_vowel_t;

/* One rendered character in the syllable currently being composed. */
typedef struct {
    uint32_t   literal;     /* ASCII consonant, or ignored for a vowel */
    vn_vowel_t vowel_type;  /* VH_NONE for a literal/consonant */
    int        tone;
} telex_token_t;

typedef struct {
    bool enabled;

    /*
     * Keep the whole current letter run, rather than only its last vowel.
     * That lets a late Telex modifier update an earlier vowel in clusters
     * such as oi (toio -> tôi) and ieu (hieues -> hiếu).
     */
    telex_token_t word[TELEX_MAX_WORD];
    int           word_len;

    /* What has already been sent to the target application. */
    uint32_t rendered[TELEX_MAX_OUTPUT];
    int      rendered_len;
    uint32_t   last_vowel_base;
    vn_vowel_t last_vowel_type;
    int        last_tone;
    uint32_t   suffix[TELEX_MAX_SUFFIX];
    int        suffix_count;
    bool       last_was_d;
    telex_token_t saved_word[TELEX_MAX_WORD];
    int           saved_word_len;
    int           saved_rendered_len;
    bool          boundary_saved;
    bool          shape_cancelled;
    telex_token_t undo_word[TELEX_MAX_WORD];
    int           undo_word_len;
    int           undo_rendered_len;
    bool          undo_valid;

    /* Debug */
    char debug_buf[64];
} telex_ctx_t;

void telex_init(telex_ctx_t *ctx);
void telex_reset(telex_ctx_t *ctx);
void telex_set_enabled(telex_ctx_t *ctx, bool enabled);

/*
 * Clear vowel tracking. Call when a non-letter key (space, punctuation,
 * backspace, digits, etc.) is passed through, so a later tone key does not
 * modify a stale vowel.
 */
void telex_reset_tracking(telex_ctx_t *ctx);
void telex_commit_boundary(telex_ctx_t *ctx);
void telex_restore_boundary(telex_ctx_t *ctx);
void telex_undo_last(telex_ctx_t *ctx);

/*
 * Process a keypress (letter key, press event only).
 * Returns a result telling the caller what to do.
 */
telex_result_t telex_process(telex_ctx_t *ctx, uint16_t keycode, bool pressed);

#endif /* TELEX_H */
