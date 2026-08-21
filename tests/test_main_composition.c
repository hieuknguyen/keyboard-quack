/* Integration regression test for composition state across Backspace. */
#include <assert.h>
#include <stdio.h>
#include "../src/engine/unicode_map.h"

/* Include the event loop with a renamed entry point so its static event
 * processor can be exercised without opening real input devices. */
#define main keyboard_quack_daemon_main
#include "../src/main.c"
#undef main

int capture_init(capture_ctx_t *ctx) { (void)ctx; return -1; }
int capture_grab(capture_ctx_t *ctx) { (void)ctx; return -1; }
int capture_ungrab(capture_ctx_t *ctx) { (void)ctx; return 0; }
int capture_read(capture_ctx_t *ctx, struct input_event *ev, int *dev_idx)
{ (void)ctx; (void)ev; (void)dev_idx; return -1; }
void capture_cleanup(capture_ctx_t *ctx) { (void)ctx; }
int capture_get_poll_fd(capture_ctx_t *ctx) { (void)ctx; return -1; }

int inject_init(inject_ctx_t *ctx) { (void)ctx; return 0; }
int inject_key(inject_ctx_t *ctx, uint16_t keycode, bool pressed)
{ (void)ctx; (void)keycode; (void)pressed; return 0; }
int inject_unicode(inject_ctx_t *ctx, uint32_t codepoint)
{ (void)ctx; (void)codepoint; return 0; }
int inject_string(inject_ctx_t *ctx, const uint32_t *codepoints, int len)
{ (void)ctx; (void)codepoints; (void)len; return 0; }
int inject_backspace(inject_ctx_t *ctx, int count)
{ (void)ctx; (void)count; return 0; }
int inject_bksp_retype(inject_ctx_t *ctx, int count,
                       const uint32_t *codepoints, int len)
{ (void)ctx; (void)count; (void)codepoints; (void)len; return 0; }
void inject_cleanup(inject_ctx_t *ctx) { (void)ctx; }

int config_load(quack_config_t *cfg, const char *path)
{ (void)cfg; (void)path; return 0; }

static void send_key(telex_ctx_t *telex, inject_ctx_t *inject,
                     uint16_t code, int value)
{
    struct input_event ev = { .type = EV_KEY, .code = code, .value = value };
    process_event(telex, inject, &ev);
}

int main(void)
{
    telex_ctx_t telex;
    inject_ctx_t inject = { 0 };
    telex_init(&telex);

    /* Test 1: Type "tôi đi" as "tooi ddi", backspace 3 times, add 's' -> "tối" */
    const uint16_t keys1[] = { 20, 24, 24, 23, KC_SPACE, 32, 32, 23 };
    for (size_t i = 0; i < sizeof(keys1) / sizeof(keys1[0]); i++) {
        send_key(&telex, &inject, keys1[i], 1);
        send_key(&telex, &inject, keys1[i], 0);
    }
    for (int i = 0; i < 3; i++) {
        send_key(&telex, &inject, KC_BACKSPACE, 1);
        send_key(&telex, &inject, KC_BACKSPACE, 0);
    }
    send_key(&telex, &inject, 31, 1); /* s */
    send_key(&telex, &inject, 31, 0);

    assert(telex.word_len == 3);
    assert(telex.word[0].literal == 't');
    assert(telex.word[1].vowel_type == VH_OCI);
    assert(telex.word[1].tone == TONE_SAC);
    assert(telex.word[2].vowel_type == VH_I);
    puts("Test 1 (backspace restore) passed.");

    /* Test 2: Word isolation across spaces */
    /* Type "xin ", then type "ch", then type "s" */
    telex_init(&telex);
    const uint16_t keys2[] = { 45, 23, 49, KC_SPACE, 46, 35, 31 }; /* x i n space c h s */
    for (size_t i = 0; i < sizeof(keys2) / sizeof(keys2[0]); i++) {
        send_key(&telex, &inject, keys2[i], 1);
        send_key(&telex, &inject, keys2[i], 0);
    }
    /* "ch" has no vowel, so 's' must be treated as literal consonant 's' in the new word "chs",
     * and MUST NOT touch "xin"! */
    assert(telex.word_len == 3);
    assert(telex.word[0].literal == 'c');
    assert(telex.word[1].literal == 'h');
    assert(telex.word[2].literal == 's');
    assert(telex.word[2].tone == TONE_NONE);
    puts("Test 2 (cross-word isolation with space) passed.");

    /* Test 3: Horn 'w' in new word does not touch previous words */
    telex_init(&telex);
    /* Type "tu ", then "con ", then "bo ", then "w" */
    /* t(20) u(22) space, c(46) o(24) n(49) space, b(48) o(24) space, w(17) */
    const uint16_t keys3[] = { 20, 22, KC_SPACE, 46, 24, 49, KC_SPACE, 48, 24, KC_SPACE, 17 };
    for (size_t i = 0; i < sizeof(keys3) / sizeof(keys3[0]); i++) {
        send_key(&telex, &inject, keys3[i], 1);
        send_key(&telex, &inject, keys3[i], 0);
    }
    /* Current word is just "w" */
    assert(telex.word_len == 1);
    assert(telex.word[0].literal == 'w');
    puts("Test 3 (horn 'w' isolation across spaces) passed.");

    /* Test 4: 'd' in new word does not convert 'd' in previous words */
    telex_init(&telex);
    /* Type "dem ", then "nay ", then "d" */
    /* d(32) e(18) m(50) space, n(49) a(30) y(21) space, d(32) */
    const uint16_t keys4[] = { 32, 18, 50, KC_SPACE, 49, 30, 21, KC_SPACE, 32 };
    for (size_t i = 0; i < sizeof(keys4) / sizeof(keys4[0]); i++) {
        send_key(&telex, &inject, keys4[i], 1);
        send_key(&telex, &inject, keys4[i], 0);
    }
    assert(telex.word_len == 1);
    assert(telex.word[0].literal == 'd'); /* Just 'd', not 'dd' or modifying 'dem' */
    puts("Test 4 ('d' isolation across spaces) passed.");

    puts("\nALL INTEGRATION TESTS PASSED!");
    return 0;
}
