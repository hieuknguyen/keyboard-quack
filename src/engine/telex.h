#ifndef TELEX_H
#define TELEX_H

#include <stdint.h>
#include <stdbool.h>

#define TELEX_MAX_OUTPUT 128
#define TELEX_MAX_WORD   128
#define TELEX_MAX_BOUNDARIES 32

/* Actions the engine tells the caller to perform */
typedef enum {
    ACT_NONE,        /* Do nothing (swallow) */
    ACT_OUTPUT,      /* Output characters in output */
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
    uint32_t   literal;     /* ASCII consonant or custom codepoint */
    vn_vowel_t vowel_type;  /* VH_NONE for a literal/consonant */
    int        tone;        /* TONE_NONE, TONE_SAC, ... */
    bool       is_upper;    /* true if uppercase */
} telex_token_t;

typedef struct {
    bool enabled;

    /*
     * Active syllable being composed (isolated to the current word).
     */
    telex_token_t word[TELEX_MAX_WORD];
    int           word_len;
    int           rendered_len;

    /* Boundary stack for backspacing across words safely */
    telex_token_t boundary_words[TELEX_MAX_BOUNDARIES][TELEX_MAX_WORD];
    int           boundary_lens[TELEX_MAX_BOUNDARIES];
    int           boundary_count;

    /* Shape cancellation (within current syllable) */
    bool          shape_cancelled;

    /* Undo support */
    telex_token_t undo_word[TELEX_MAX_WORD];
    int           undo_word_len;
    int           undo_rendered_len;
    bool          undo_valid;

    /* Deleted token support */
    telex_token_t deleted_token;
    bool          deleted_token_valid;
} telex_ctx_t;

void telex_init(telex_ctx_t *ctx);
void telex_reset(telex_ctx_t *ctx);
void telex_set_enabled(telex_ctx_t *ctx, bool enabled);

/*
 * Clear vowel tracking. Call when a non-letter key or focus change occurs.
 */
void telex_reset_tracking(telex_ctx_t *ctx);

/*
 * Commit the current word to history and reset active buffer.
 */
void telex_commit_word(telex_ctx_t *ctx);

/*
 * Handle Backspace key on the composition buffer.
 */
void telex_handle_backspace(telex_ctx_t *ctx);

void telex_undo_last(telex_ctx_t *ctx);

/*
 * Process a keypress (letter key, press event only).
 * Returns a result telling the caller what to do.
 */
telex_result_t telex_process(telex_ctx_t *ctx, uint16_t keycode, bool pressed, bool is_upper);

#endif /* TELEX_H */
