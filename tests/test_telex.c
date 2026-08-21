/* Quick test for the telex engine - feeds key sequences and prints output */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "engine/telex.h"
#include "engine/unicode_map.h"

/* Map keycodes: q-p = 16-25, a-l = 30-38, z-m = 44-50 */
static uint16_t kc(char c)
{
    if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
    switch (c) {
    case 'q': return 16; case 'w': return 17; case 'e': return 18;
    case 'r': return 19; case 't': return 20; case 'y': return 21;
    case 'u': return 22; case 'i': return 23; case 'o': return 24;
    case 'p': return 25;
    case 'a': return 30; case 's': return 31; case 'd': return 32;
    case 'f': return 33; case 'g': return 34; case 'h': return 35;
    case 'j': return 36; case 'k': return 37; case 'l': return 38;
    case 'z': return 44; case 'x': return 45; case 'c': return 46;
    case 'v': return 47; case 'b': return 48; case 'n': return 49;
    case 'm': return 50;
    }
    return 0;
}

static void utf8(uint32_t cp, char *out)
{
    if (cp < 0x80) {
        out[0] = (char)cp; out[1] = 0;
    } else if (cp < 0x800) {
        out[0] = 0xC0 | (cp >> 6);
        out[1] = 0x80 | (cp & 0x3F);
        out[2] = 0;
    } else {
        out[0] = 0xE0 | (cp >> 12);
        out[1] = 0x80 | ((cp >> 6) & 0x3F);
        out[2] = 0x80 | (cp & 0x3F);
        out[3] = 0;
    }
}

static void type_str(telex_ctx_t *ctx, const char *s)
{
    printf("  typing \"%s\":\n", s);
    for (const char *p = s; *p; p++) {
        if (*p == ' ') {
            telex_commit_word(ctx);
            printf("    ' ' -> (commit)\n");
            continue;
        }
        uint16_t key = kc(*p);
        bool is_upper = (*p >= 'A' && *p <= 'Z');
        telex_result_t r = telex_process(ctx, key, true, is_upper);
        char buf[16];
        switch (r.action) {
        case ACT_OUTPUT:
            printf("    '%c' -> out:", *p);
            for (int i = 0; i < r.output_len; i++) {
                utf8(r.output[i], buf);
                printf(" %s", buf);
            }
            printf("\n");
            break;
        case ACT_BKSP_OUTPUT:
            printf("    '%c' -> bksp=%d out:", *p, r.backspace_count);
            for (int i = 0; i < r.output_len; i++) {
                utf8(r.output[i], buf);
                printf(" %s", buf);
            }
            printf("\n");
            break;
        default:
            printf("    '%c' -> (none)\n", *p);
        }
    }
    printf("    ---\n");
}

int main(void)
{
    telex_ctx_t ctx;
    telex_init(&ctx);

    printf("=== TEST: basic vowels ===\n");
    type_str(&ctx, "tam");
    telex_init(&ctx);
    type_str(&ctx, "tams");
    telex_init(&ctx);
    type_str(&ctx, "taam");
    telex_init(&ctx);
    type_str(&ctx, "taams");
    telex_init(&ctx);
    type_str(&ctx, "thuong");
    telex_init(&ctx);
    type_str(&ctx, "ddi");
    telex_init(&ctx);
    type_str(&ctx, "hoc");
    telex_init(&ctx);
    type_str(&ctx, "xe");
    telex_init(&ctx);
    type_str(&ctx, "xeen");

    printf("\n=== TEST: sentences without reset (word isolation) ===\n");
    telex_init(&ctx);
    type_str(&ctx, "xin chao cac ban");
    type_str(&ctx, "toi ddi hoc ve");
    type_str(&ctx, "con meof mowis");

    printf("\n=== TEST: capitalization ===\n");
    telex_init(&ctx);
    type_str(&ctx, "Tooi");
    telex_init(&ctx);
    type_str(&ctx, "VIET");
    telex_init(&ctx);
    type_str(&ctx, "DDi");

    printf("\n=== TEST: tone at end ===\n");
    telex_init(&ctx);
    type_str(&ctx, "hoctap");
    telex_init(&ctx);
    type_str(&ctx, "phams");
    telex_init(&ctx);
    type_str(&ctx, "vietnam");

    printf("\n=== TEST: complex ===\n");
    telex_init(&ctx);
    type_str(&ctx, "nghiep");
    telex_init(&ctx);
    type_str(&ctx, "nguoi");
    telex_init(&ctx);
    type_str(&ctx, "uong");
    telex_init(&ctx);
    type_str(&ctx, "quoc");

    return 0;
}