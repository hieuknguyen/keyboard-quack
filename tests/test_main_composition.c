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
int inject_key_val(inject_ctx_t *ctx, uint16_t keycode, int val)
{ (void)ctx; (void)keycode; (void)val; return 0; }
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
    process_event(telex, inject, &ev, false);
}

static void send_mouse_click(telex_ctx_t *telex, inject_ctx_t *inject, uint16_t code)
{
    struct input_event ev = { .type = EV_KEY, .code = code, .value = 1 };
    process_event(telex, inject, &ev, true);
    ev.value = 0;
    process_event(telex, inject, &ev, true);
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

    /* Test 5: Backspacing over cancelling key restores cancellation state so tone can be toggled */
    telex_init(&telex);
    /* Type "hieeus" -> "hiếu" */
    /* h(35) i(23) e(18) e(18) u(22) s(31) */
    const uint16_t keys5[] = { 35, 23, 18, 18, 22, 31 };
    for (size_t i = 0; i < sizeof(keys5) / sizeof(keys5[0]); i++) {
        send_key(&telex, &inject, keys5[i], 1);
        send_key(&telex, &inject, keys5[i], 0);
    }
    /* Type 'e' -> "hiéue" (cancelled shape) */
    send_key(&telex, &inject, 18, 1);
    send_key(&telex, &inject, 18, 0);
    assert(telex.shape_cancelled == true);

    /* Backspace over 'e' -> "hiéu" (shape_cancelled should be reset to false) */
    send_key(&telex, &inject, KC_BACKSPACE, 1);
    send_key(&telex, &inject, KC_BACKSPACE, 0);
    assert(telex.shape_cancelled == false);

    /* Type 's' -> tone on 'e' is cancelled, yielding "hieus" */
    send_key(&telex, &inject, 31, 1);
    send_key(&telex, &inject, 31, 0);
    assert(telex.word_len == 5);
    assert(telex.word[0].literal == 'h');
    assert(telex.word[1].vowel_type == VH_I);
    assert(telex.word[2].vowel_type == VH_E && telex.word[2].tone == TONE_NONE);
    assert(telex.word[3].vowel_type == VH_U);
    assert(telex.word[4].literal == 's');
    puts("Test 5 (shape cancellation reset on backspace & tone toggle) passed.");

    /* Test 6: Holding Backspace back to a previous word allows tone modification on that word */
    telex_init(&telex);
    /* Type "tooi hoc" */
    /* t(20) o(24) o(24) i(23) space, h(35) o(24) c(46) */
    const uint16_t keys6[] = { 20, 24, 24, 23, KC_SPACE, 35, 24, 46 };
    for (size_t i = 0; i < sizeof(keys6) / sizeof(keys6[0]); i++) {
        send_key(&telex, &inject, keys6[i], 1);
        send_key(&telex, &inject, keys6[i], 0);
    }
    assert(telex.boundary_count == 1);
    assert(telex.word_len == 3);

    /* Hold Backspace: 1 press + 3 repeats = 4 backspaces total (deletes 'c', 'o', 'h', and the space) */
    struct input_event ev_bsp_press = { .type = EV_KEY, .code = KC_BACKSPACE, .value = 1 };
    process_event(&telex, &inject, &ev_bsp_press, false);
    struct input_event ev_bsp_rep = { .type = EV_KEY, .code = KC_BACKSPACE, .value = 2 };
    for (int i = 0; i < 3; i++) {
        process_event(&telex, &inject, &ev_bsp_rep, false);
    }
    struct input_event ev_bsp_rel = { .type = EV_KEY, .code = KC_BACKSPACE, .value = 0 };
    process_event(&telex, &inject, &ev_bsp_rel, false);

    /* "tooi" ("tôi") must be restored in buffer */
    assert(telex.word_len == 3);
    assert(telex.word[0].literal == 't');
    assert(telex.word[1].vowel_type == VH_OCI);
    assert(telex.word[2].vowel_type == VH_I);

    /* Typing 's' should apply tone to 'ô' -> "tối" */
    send_key(&telex, &inject, 31, 1); send_key(&telex, &inject, 31, 0); // s
    assert(telex.word[1].tone == TONE_SAC);

    /* Now hold Backspace to erase all of "tối" (1 press + 3 repeats = 4 backspaces) */
    process_event(&telex, &inject, &ev_bsp_press, false);
    for (int i = 0; i < 3; i++) {
        process_event(&telex, &inject, &ev_bsp_rep, false);
    }
    process_event(&telex, &inject, &ev_bsp_rel, false);

    /* Buffer must now be completely clean */
    assert(telex.word_len == 0);
    assert(telex.boundary_count == 0);

    /* Typing new word "day" should be completely fresh and isolated */
    send_key(&telex, &inject, 32, 1); send_key(&telex, &inject, 32, 0); // d
    send_key(&telex, &inject, 30, 1); send_key(&telex, &inject, 30, 0); // a
    send_key(&telex, &inject, 21, 1); send_key(&telex, &inject, 21, 0); // y
    assert(telex.word_len == 3);
    assert(telex.word[0].literal == 'd');
    assert(telex.word[1].vowel_type == VH_A);
    assert(telex.word[2].vowel_type == VH_Y);
    puts("Test 6 (held backspace word restore & tone processing) passed.");

    /* Test 7: Horn toggle 'a'+'w'+'w' -> 'aw' and 'w' at start of word as literal */
    telex_init(&telex);
    /* 'w' at start of word is literal 'w' */
    send_key(&telex, &inject, 17, 1); send_key(&telex, &inject, 17, 0); // w
    assert(telex.word_len == 1);
    assert(telex.word[0].literal == 'w' && telex.word[0].vowel_type == VH_NONE);

    /* Horn toggle: 'a' + 'w' -> 'ă', + 'w' -> 'aw' */
    telex_init(&telex);
    send_key(&telex, &inject, 30, 1); send_key(&telex, &inject, 30, 0); // a
    send_key(&telex, &inject, 17, 1); send_key(&telex, &inject, 17, 0); // w -> ă
    assert(telex.word_len == 1 && telex.word[0].vowel_type == VH_ACR);
    send_key(&telex, &inject, 17, 1); send_key(&telex, &inject, 17, 0); // w -> aw
    assert(telex.word_len == 2 && telex.word[0].vowel_type == VH_A && telex.word[1].literal == 'w');
    puts("Test 7 (horn toggle) passed.");

    /* Test 8: Letter autorepeat (val == 2) */
    telex_init(&telex);
    struct input_event ev_a = { .type = EV_KEY, .code = 30, .value = 1 }; // a
    process_event(&telex, &inject, &ev_a, false);
    ev_a.value = 2; // repeat -> â
    process_event(&telex, &inject, &ev_a, false);
    assert(telex.word_len == 1 && telex.word[0].vowel_type == VH_ACI);
    process_event(&telex, &inject, &ev_a, false); // repeat -> aa
    assert(telex.word_len == 2 && telex.shape_cancelled == true);
    process_event(&telex, &inject, &ev_a, false); // repeat -> aaa
    assert(telex.word_len == 3);
    puts("Test 8 (letter autorepeat) passed.");

    /* Test 9: Fix 1 - Tone placement on closed diphthongs oa/oe/uy (toán, hoàng, khoác) */
    telex_init(&telex);
    /* Type "toans" -> "toán": t(20) o(24) a(30) n(49) s(31) */
    const uint16_t keys9_1[] = { 20, 24, 30, 49, 31 };
    for (size_t i = 0; i < sizeof(keys9_1) / sizeof(keys9_1[0]); i++) {
        send_key(&telex, &inject, keys9_1[i], 1);
        send_key(&telex, &inject, keys9_1[i], 0);
    }
    assert(telex.word_len == 4);
    assert(telex.word[0].literal == 't');
    assert(telex.word[1].vowel_type == VH_O && telex.word[1].tone == TONE_NONE);
    assert(telex.word[2].vowel_type == VH_A && telex.word[2].tone == TONE_SAC);
    assert(telex.word[3].literal == 'n');

    /* Type "hoangf" -> "hoàng": h(35) o(24) a(30) n(49) g(34) f(33) */
    telex_init(&telex);
    const uint16_t keys9_2[] = { 35, 24, 30, 49, 34, 33 };
    for (size_t i = 0; i < sizeof(keys9_2) / sizeof(keys9_2[0]); i++) {
        send_key(&telex, &inject, keys9_2[i], 1);
        send_key(&telex, &inject, keys9_2[i], 0);
    }
    assert(telex.word_len == 5);
    assert(telex.word[1].vowel_type == VH_O && telex.word[1].tone == TONE_NONE);
    assert(telex.word[2].vowel_type == VH_A && telex.word[2].tone == TONE_HUYEN);

    /* Type "toasn" -> "toán": t(20) o(24) a(30) s(31) n(49) */
    telex_init(&telex);
    const uint16_t keys9_3[] = { 20, 24, 30, 31, 49 };
    for (size_t i = 0; i < sizeof(keys9_3) / sizeof(keys9_3[0]); i++) {
        send_key(&telex, &inject, keys9_3[i], 1);
        send_key(&telex, &inject, keys9_3[i], 0);
    }
    assert(telex.word_len == 4);
    assert(telex.word[1].vowel_type == VH_O && telex.word[1].tone == TONE_NONE);
    assert(telex.word[2].vowel_type == VH_A && telex.word[2].tone == TONE_SAC);
    assert(telex.word[3].literal == 'n');
    puts("Test 9 (tone placement on oa/oe/uy with coda) passed.");

    /* Test 10: Fix 2 - Tone placement for 'gi' + vowels (giá, già, giảm, giẻ) */
    telex_init(&telex);
    /* Type "gias" -> "giá": g(34) i(23) a(30) s(31) */
    const uint16_t keys10_1[] = { 34, 23, 30, 31 };
    for (size_t i = 0; i < sizeof(keys10_1) / sizeof(keys10_1[0]); i++) {
        send_key(&telex, &inject, keys10_1[i], 1);
        send_key(&telex, &inject, keys10_1[i], 0);
    }
    assert(telex.word_len == 3);
    assert(telex.word[0].literal == 'g');
    assert(telex.word[1].vowel_type == VH_I && telex.word[1].tone == TONE_NONE);
    assert(telex.word[2].vowel_type == VH_A && telex.word[2].tone == TONE_SAC);

    /* Type "giamr" -> "giảm": g(34) i(23) a(30) m(50) r(19) */
    telex_init(&telex);
    const uint16_t keys10_2[] = { 34, 23, 30, 50, 19 };
    for (size_t i = 0; i < sizeof(keys10_2) / sizeof(keys10_2[0]); i++) {
        send_key(&telex, &inject, keys10_2[i], 1);
        send_key(&telex, &inject, keys10_2[i], 0);
    }
    assert(telex.word_len == 4);
    assert(telex.word[1].vowel_type == VH_I && telex.word[1].tone == TONE_NONE);
    assert(telex.word[2].vowel_type == VH_A && telex.word[2].tone == TONE_HOI);
    puts("Test 10 (tone placement on gi+vowels) passed.");

    /* Test 11: Fix 3 - 'quow' -> 'quơ' (not 'qươ') and 'quowr' -> 'quở' */
    telex_init(&telex);
    /* Type "quowr" -> "quở": q(16) u(22) o(24) w(17) r(19) */
    const uint16_t keys11[] = { 16, 22, 24, 17, 19 };
    for (size_t i = 0; i < sizeof(keys11) / sizeof(keys11[0]); i++) {
        send_key(&telex, &inject, keys11[i], 1);
        send_key(&telex, &inject, keys11[i], 0);
    }
    assert(telex.word_len == 3);
    assert(telex.word[0].literal == 'q');
    assert(telex.word[1].vowel_type == VH_U && telex.word[1].tone == TONE_NONE);
    assert(telex.word[2].vowel_type == VH_OHR && telex.word[2].tone == TONE_HOI);
    puts("Test 11 (quow -> quơ not qươ) passed.");

    /* Test 12: Fix 4 - 'z' clears tone in Telex */
    telex_init(&telex);
    /* Type "toans" -> "toán", then press 'z'(44) -> "toan" */
    const uint16_t keys12_1[] = { 20, 24, 30, 49, 31, 44 };
    for (size_t i = 0; i < sizeof(keys12_1) / sizeof(keys12_1[0]); i++) {
        send_key(&telex, &inject, keys12_1[i], 1);
        send_key(&telex, &inject, keys12_1[i], 0);
    }
    assert(telex.word_len == 4);
    assert(telex.word[1].tone == TONE_NONE && telex.word[2].tone == TONE_NONE);
    puts("Test 12 ('z' clears tone) passed.");

    /* Test 13: Mouse click resets tracking (switching window mid-word) */
    telex_init(&telex);
    /* In App A: type "to" -> t(20) o(24) */
    send_key(&telex, &inject, 20, 1); send_key(&telex, &inject, 20, 0);
    send_key(&telex, &inject, 24, 1); send_key(&telex, &inject, 24, 0);
    assert(telex.word_len == 2);
    /* User clicks mouse on App B */
    send_mouse_click(&telex, &inject, BTN_LEFT);
    assert(telex.word_len == 0);
    /* In App B: user types 'a' (30) */
    send_key(&telex, &inject, 30, 1); send_key(&telex, &inject, 30, 0);
    assert(telex.word_len == 1);
    assert(telex.word[0].literal == 'a');
    puts("Test 13 (mouse click resets tracking across apps) passed.");

    /* Test 14: Navigation key (Arrow, Enter, Tab, Esc) resets tracking */
    telex_init(&telex);
    /* Type "to", press Right Arrow (106), type "s" */
    send_key(&telex, &inject, 20, 1); send_key(&telex, &inject, 20, 0);
    send_key(&telex, &inject, 24, 1); send_key(&telex, &inject, 24, 0);
    assert(telex.word_len == 2);
    send_key(&telex, &inject, 106, 1); send_key(&telex, &inject, 106, 0); /* Right Arrow */
    assert(telex.word_len == 0);
    /* Type "s" (31) in new position */
    send_key(&telex, &inject, 31, 1); send_key(&telex, &inject, 31, 0);
    assert(telex.word_len == 1);
    assert(telex.word[0].literal == 's');
    puts("Test 14 (navigation keys reset tracking) passed.");

    /* Test 15: Alt / Super modifier resets tracking */
    telex_init(&telex);
    /* Type "to", press Alt (56) (e.g. Alt+Tab) */
    send_key(&telex, &inject, 20, 1); send_key(&telex, &inject, 20, 0);
    send_key(&telex, &inject, 24, 1); send_key(&telex, &inject, 24, 0);
    assert(telex.word_len == 2);
    send_key(&telex, &inject, 56, 1); send_key(&telex, &inject, 56, 0); /* Alt */
    assert(telex.word_len == 0);
    puts("Test 15 (Alt/Super modifier resets tracking) passed.");

    puts("\nALL INTEGRATION TESTS PASSED!");
    return 0;
}
