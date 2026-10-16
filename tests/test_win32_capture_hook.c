/* Windows hook regressions. Build with MinGW on Windows. */
#include "../src/capture/win32_capture.c"
#include <stdio.h>
#include <wchar.h>
#include <wtsapi32.h>

static WCHAR screen_text[256];
static int screen_length;

static void screen_append(uint32_t codepoint)
{
    if (codepoint <= 0xFFFF && screen_length < 255)
        screen_text[screen_length++] = (WCHAR)codepoint;
    screen_text[screen_length] = L'\0';
}

static void screen_backspace(int count)
{
    while (count-- > 0 && screen_length > 0) screen_length--;
    screen_text[screen_length] = L'\0';
}

int win32_inject_init(win32_inject_ctx_t *ctx) { (void)ctx; return 0; }
int win32_inject_unicode(win32_inject_ctx_t *ctx, uint32_t codepoint)
{ (void)ctx; screen_append(codepoint); return 0; }
int win32_inject_string(win32_inject_ctx_t *ctx,
                        const uint32_t *codepoints, int len)
{
    (void)ctx;
    for (int i = 0; codepoints && i < len; i++) screen_append(codepoints[i]);
    return 0;
}
int win32_inject_backspace(win32_inject_ctx_t *ctx, int count)
{ (void)ctx; screen_backspace(count); return 0; }
int win32_inject_bksp_retype(win32_inject_ctx_t *ctx, int count,
                             const uint32_t *codepoints, int len)
{
    win32_inject_backspace(ctx, count);
    return win32_inject_string(ctx, codepoints, len);
}
void win32_inject_cleanup(win32_inject_ctx_t *ctx) { (void)ctx; }

static LRESULT send_vk(DWORD vk, bool down)
{
    KBDLLHOOKSTRUCT key;
    ZeroMemory(&key, sizeof(key));
    key.vkCode = vk;
    LRESULT result = LowLevelKeyboardProc(HC_ACTION,
                         down ? WM_KEYDOWN : WM_KEYUP, (LPARAM)&key);
    if (down && result != 1) {
        if (vk == VK_SPACE) screen_append(' ');
        else if (vk == VK_BACK) screen_backspace(1);
        else if (vk >= 'A' && vk <= 'Z') screen_append(vk + ('a' - 'A'));
    }
    return result;
}

static void type_vk(DWORD vk)
{
    send_vk(vk, true);
    send_vk(vk, false);
}

static void reset_text_and_composition(telex_ctx_t *telex)
{
    screen_length = 0;
    screen_text[0] = L'\0';
    telex_reset(telex);
}

int main(void)
{
    telex_ctx_t telex;
    quack_config_t config;
    win32_inject_ctx_t inject;
    win32_capture_ctx_t ctx;
    telex_init(&telex);
    config_defaults(&config);
    ZeroMemory(&inject, sizeof(inject));
    ZeroMemory(&ctx, sizeof(ctx));
    ctx.tctx = &telex;
    ctx.ictx = &inject;
    ctx.config = &config;
    ctx.vn_enabled = 1;
    ctx.foreground_identity_initialized = true;
    ctx.foreground_window = GetForegroundWindow();
    GetWindowThreadProcessId(ctx.foreground_window, &ctx.foreground_process_id);
    g_capture_ctx = &ctx;
    if (!config_parse_toggle_shortcut(&config, "Ctrl+Space")) return 1;

    /* Ctrl+Shift+Esc must reach Windows if the app has not claimed Ctrl+Shift. */
    send_vk(VK_LCONTROL, true);
    LRESULT shift_result = send_vk(VK_LSHIFT, true);
    LRESULT escape_result = send_vk(VK_ESCAPE, true);
    send_vk(VK_ESCAPE, false);
    send_vk(VK_LSHIFT, false);
    send_vk(VK_LCONTROL, false);
    if (shift_result == 1 || escape_result == 1) {
        fprintf(stderr, "Ctrl+Shift+Esc was swallowed (shift=%ld, esc=%ld).\n",
                (long)shift_result, (long)escape_result);
        return 1;
    }
    puts("Ctrl+Shift+Esc pass-through passed.");

    /* Win+Space is also Windows' layout shortcut unless assigned to the app. */
    send_vk(VK_LWIN, true);
    LRESULT space_result = send_vk(VK_SPACE, true);
    send_vk(VK_SPACE, false);
    send_vk(VK_LWIN, false);
    if (space_result == 1) {
        fputs("Unassigned Win+Space was swallowed.\n", stderr);
        return 1;
    }
    puts("Unassigned Win+Space pass-through passed.");

    /* A Ctrl+Shift shortcut explicitly assigned to the app must still toggle it. */
    if (!config_parse_toggle_shortcut(&config, "Ctrl+Shift+K")) return 1;
    send_vk(VK_LCONTROL, true);
    send_vk(VK_LSHIFT, true);
    send_vk('K', true);
    send_vk('K', false);
    send_vk(VK_LSHIFT, false);
    send_vk(VK_LCONTROL, false);
    if (ctx.vn_enabled != 0) {
        fputs("Configured Ctrl+Shift+K did not toggle Vietnamese.\n", stderr);
        return 1;
    }
    puts("Configured Ctrl+Shift+K toggle passed.");

    config_defaults(&config);
    ctx.vn_enabled = 1;
    telex_set_enabled(&telex, true);

    /* Deleting the last word and its separator must not recreate that space. */
    reset_text_and_composition(&telex);
    type_vk('A');
    type_vk(VK_SPACE);
    type_vk('B');
    type_vk(VK_BACK);
    type_vk(VK_BACK);
    type_vk('C');
    if (wcscmp(screen_text, L"ac") != 0) {
        fwprintf(stderr, L"Deleted separator was reinserted: '%ls'.\n", screen_text);
        return 1;
    }
    puts("No space reinserted after deleting a word and its separator.");

    /* Deleting only the word keeps its original separator in the target. */
    reset_text_and_composition(&telex);
    type_vk('A');
    type_vk(VK_SPACE);
    type_vk('B');
    type_vk(VK_BACK);
    type_vk('C');
    if (wcscmp(screen_text, L"a c") != 0) {
        fwprintf(stderr, L"Existing separator was lost or duplicated: '%ls'.\n",
                 screen_text);
        return 1;
    }
    puts("Existing space remains when only the word is deleted.");

    /* Repeated u plus w places the horn on the first u: luuw -> lưu. */
    reset_text_and_composition(&telex);
    type_vk('L');
    type_vk('U');
    type_vk('U');
    type_vk('W');
    if (wcscmp(screen_text, L"l\u01b0u") != 0) {
        fwprintf(stderr, L"luuw should produce lưu, got '%ls'.\n", screen_text);
        return 1;
    }
    puts("luuw produces lưu.");

    /* uô plus w reshapes both vowels to ươ. */
    reset_text_and_composition(&telex);
    type_vk('U');
    type_vk('O');
    type_vk('O');
    type_vk('W');
    if (wcscmp(screen_text, L"\u01b0\u01a1") != 0) {
        fwprintf(stderr, L"uôw should produce ươ, got '%ls'.\n", screen_text);
        return 1;
    }
    puts("uôw produces ươ.");

    /* w changes â to ă instead of producing a literal w. */
    reset_text_and_composition(&telex);
    type_vk('A');
    type_vk('A');
    type_vk('W');
    if (wcscmp(screen_text, L"\u0103") != 0) {
        fwprintf(stderr, L"aaw should produce ă, got '%ls'.\n", screen_text);
        return 1;
    }
    puts("aaw produces ă.");

    /* Repeating a after ă switches it to â. */
    reset_text_and_composition(&telex);
    type_vk('A');
    type_vk('W');
    type_vk('A');
    if (wcscmp(screen_text, L"\u00e2") != 0) {
        fwprintf(stderr, L"awa should produce â, got '%ls'.\n", screen_text);
        return 1;
    }
    puts("awa produces â.");

    /* w after ê remains literal, rather than changing or swallowing it. */
    reset_text_and_composition(&telex);
    type_vk('E');
    type_vk('E');
    type_vk('W');
    if (wcscmp(screen_text, L"\u00eaw") != 0) {
        fwprintf(stderr, L"eew should produce êw, got '%ls'.\n", screen_text);
        return 1;
    }
    puts("eew produces êw.");

    /* w changes ô to ơ; o changes it back to ô. */
    reset_text_and_composition(&telex);
    type_vk('O');
    type_vk('O');
    type_vk('W');
    if (wcscmp(screen_text, L"\u01a1") != 0) {
        fwprintf(stderr, L"oow should produce ơ, got '%ls'.\n", screen_text);
        return 1;
    }
    reset_text_and_composition(&telex);
    type_vk('O');
    type_vk('W');
    type_vk('O');
    if (wcscmp(screen_text, L"\u00f4") != 0) {
        fwprintf(stderr, L"owo should produce ô, got '%ls'.\n", screen_text);
        return 1;
    }
    puts("oow produces ơ and owo produces ô.");

    /* Win+L can switch to a secure desktop before the hook sees Win key-up. */
    reset_text_and_composition(&telex);
    send_vk(VK_LWIN, true);
    send_vk('L', true);
    send_vk('L', false);
    ctx.physical_right_ctrl = true;
    ctx.physical_left_shift = true;
    ctx.physical_left_alt = true;
    ctx.physical_right_win = true;
    ctx.ctrl_shift_latched = 1;
    ctx.backspace_keydown_swallowed = true;
    ctx.letter_keydown_swallowed[0] = true;
    ctx.shortcut_keydown_swallowed[VK_ESCAPE] = true;
    ctx.suppress_modifier_keyup[VK_LWIN] = true;

    TrayWindowProc(NULL, WM_WTSSESSION_CHANGE, WTS_SESSION_LOCK, 0);
    if (ctx.physical_left_win || ctx.physical_right_win ||
        ctx.physical_right_ctrl || ctx.physical_left_shift ||
        ctx.physical_left_alt || ctx.ctrl_shift_latched != 0 ||
        ctx.backspace_keydown_swallowed ||
        ctx.letter_keydown_swallowed[0] ||
        ctx.shortcut_keydown_swallowed[VK_ESCAPE] ||
        ctx.suppress_modifier_keyup[VK_LWIN]) {
        fputs("Session lock did not clear stale keyboard state.\n", stderr);
        return 1;
    }

    ctx.physical_left_win = true;
    TrayWindowProc(NULL, WM_WTSSESSION_CHANGE, WTS_SESSION_UNLOCK, 0);
    if (ctx.physical_left_win || send_vk('A', true) != 1) {
        fputs("Vietnamese processing did not resume after session unlock.\n",
              stderr);
        return 1;
    }
    puts("Win+L session transitions clear stale key state and resume Telex.");
    return 0;
}
