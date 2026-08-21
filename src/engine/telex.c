#include "telex.h"
#include "unicode_map.h"
#include <string.h>

/* Linux input keycodes */
#define KEY_A 30
#define KEY_E 18
#define KEY_O 24
#define KEY_U 22
#define KEY_I 23
#define KEY_Y 21
#define KEY_W 17
#define KEY_D 32
#define KEY_S 31
#define KEY_F 33
#define KEY_R 19
#define KEY_X 45
#define KEY_Z 44
#define KEY_J 36

static char keycode_to_char(uint16_t kc)
{
    switch (kc) {
    case 16: return 'q'; case 17: return 'w'; case 18: return 'e';
    case 19: return 'r'; case 20: return 't'; case 21: return 'y';
    case 22: return 'u'; case 23: return 'i'; case 24: return 'o';
    case 25: return 'p';
    case 30: return 'a'; case 31: return 's'; case 32: return 'd';
    case 33: return 'f'; case 34: return 'g'; case 35: return 'h';
    case 36: return 'j'; case 37: return 'k'; case 38: return 'l';
    case 44: return 'z'; case 45: return 'x'; case 46: return 'c';
    case 47: return 'v'; case 48: return 'b'; case 49: return 'n';
    case 50: return 'm';
    }
    return 0;
}

/* Base codepoint (no tone) for each vowel type */
static uint32_t vowel_base_cp(vn_vowel_t vt, bool is_upper)
{
    if (is_upper) {
        switch (vt) {
        case VH_A:   return VN_A;
        case VH_ACR: return VN_Acr;
        case VH_ACI: return VN_Aci;
        case VH_E:   return VN_E;
        case VH_ECI: return VN_Eci;
        case VH_O:   return VN_O;
        case VH_OCI: return VN_Oci;
        case VH_OHR: return VN_Ohr;
        case VH_U:   return VN_U;
        case VH_UHR: return VN_Uhr;
        case VH_I:   return VN_I;
        case VH_Y:   return VN_Y;
        default:     return 0;
        }
    } else {
        switch (vt) {
        case VH_A:   return VN_a;
        case VH_ACR: return VN_acr;
        case VH_ACI: return VN_aci;
        case VH_E:   return VN_e;
        case VH_ECI: return VN_eci;
        case VH_O:   return VN_o;
        case VH_OCI: return VN_oci;
        case VH_OHR: return VN_ohr;
        case VH_U:   return VN_u;
        case VH_UHR: return VN_uhr;
        case VH_I:   return VN_i;
        case VH_Y:   return VN_y;
        default:     return 0;
        }
    }
}

static vn_vowel_t kc_to_vowel_type(uint16_t kc)
{
    switch (kc) {
    case KEY_A: return VH_A;
    case KEY_E: return VH_E;
    case KEY_O: return VH_O;
    case KEY_U: return VH_U;
    case KEY_I: return VH_I;
    case KEY_Y: return VH_Y;
    default:    return VH_NONE;
    }
}

/* Apply a tone to a base vowel codepoint */
static uint32_t apply_tone(uint32_t base, int tone, bool is_upper)
{
    if (tone == TONE_NONE) return base;
    if (is_upper) {
        switch (base) {
        case VN_A:
            switch (tone) {
            case TONE_SAC:   return VN_A_sac;
            case TONE_HUYEN: return VN_A_huyen;
            case TONE_HOI:   return VN_A_hoi;
            case TONE_NGA:   return VN_A_ngai;
            case TONE_NANG:  return VN_A_nang;
            }
            return VN_A;
        case VN_Acr:
            switch (tone) {
            case TONE_SAC:   return VN_Acr_sac;
            case TONE_HUYEN: return VN_Acr_huyen;
            case TONE_HOI:   return VN_Acr_hoi;
            case TONE_NGA:   return VN_Acr_ngai;
            case TONE_NANG:  return VN_Acr_nang;
            }
            return VN_Acr;
        case VN_Aci:
            switch (tone) {
            case TONE_SAC:   return VN_Aci_sac;
            case TONE_HUYEN: return VN_Aci_huyen;
            case TONE_HOI:   return VN_Aci_hoi;
            case TONE_NGA:   return VN_Aci_ngai;
            case TONE_NANG:  return VN_Aci_nang;
            }
            return VN_Aci;
        case VN_E:
            switch (tone) {
            case TONE_SAC:   return VN_E_sac;
            case TONE_HUYEN: return VN_E_huyen;
            case TONE_HOI:   return VN_E_hoi;
            case TONE_NGA:   return VN_E_ngai;
            case TONE_NANG:  return VN_E_nang;
            }
            return VN_E;
        case VN_Eci:
            switch (tone) {
            case TONE_SAC:   return VN_Eci_sac;
            case TONE_HUYEN: return VN_Eci_huyen;
            case TONE_HOI:   return VN_Eci_hoi;
            case TONE_NGA:   return VN_Eci_ngai;
            case TONE_NANG:  return VN_Eci_nang;
            }
            return VN_Eci;
        case VN_O:
            switch (tone) {
            case TONE_SAC:   return VN_O_sac;
            case TONE_HUYEN: return VN_O_huyen;
            case TONE_HOI:   return VN_O_hoi;
            case TONE_NGA:   return VN_O_ngai;
            case TONE_NANG:  return VN_O_nang;
            }
            return VN_O;
        case VN_Oci:
            switch (tone) {
            case TONE_SAC:   return VN_Oci_sac;
            case TONE_HUYEN: return VN_Oci_huyen;
            case TONE_HOI:   return VN_Oci_hoi;
            case TONE_NGA:   return VN_Oci_ngai;
            case TONE_NANG:  return VN_Oci_nang;
            }
            return VN_Oci;
        case VN_Ohr:
            switch (tone) {
            case TONE_SAC:   return VN_Ohr_sac;
            case TONE_HUYEN: return VN_Ohr_huyen;
            case TONE_HOI:   return VN_Ohr_hoi;
            case TONE_NGA:   return VN_Ohr_ngai;
            case TONE_NANG:  return VN_Ohr_nang;
            }
            return VN_Ohr;
        case VN_U:
            switch (tone) {
            case TONE_SAC:   return VN_U_sac;
            case TONE_HUYEN: return VN_U_huyen;
            case TONE_HOI:   return VN_U_hoi;
            case TONE_NGA:   return VN_U_ngai;
            case TONE_NANG:  return VN_U_nang;
            }
            return VN_U;
        case VN_Uhr:
            switch (tone) {
            case TONE_SAC:   return VN_Uhr_sac;
            case TONE_HUYEN: return VN_Uhr_huyen;
            case TONE_HOI:   return VN_Uhr_hoi;
            case TONE_NGA:   return VN_Uhr_ngai;
            case TONE_NANG:  return VN_Uhr_nang;
            }
            return VN_Uhr;
        case VN_I:
            switch (tone) {
            case TONE_SAC:   return VN_I_sac;
            case TONE_HUYEN: return VN_I_huyen;
            case TONE_HOI:   return VN_I_hoi;
            case TONE_NGA:   return VN_I_ngai;
            case TONE_NANG:  return VN_I_nang;
            }
            return VN_I;
        case VN_Y:
            switch (tone) {
            case TONE_SAC:   return VN_Y_sac;
            case TONE_HUYEN: return VN_Y_huyen;
            case TONE_HOI:   return VN_Y_hoi;
            case TONE_NGA:   return VN_Y_ngai;
            case TONE_NANG:  return VN_Y_nang;
            }
            return VN_Y;
        }
        return base;
    } else {
        switch (base) {
        case VN_a:
            switch (tone) {
            case TONE_SAC:   return VN_a_sac;
            case TONE_HUYEN: return VN_a_huyen;
            case TONE_HOI:   return VN_a_hoi;
            case TONE_NGA:   return VN_a_ngai;
            case TONE_NANG:  return VN_a_nang;
            }
            return VN_a;
        case VN_acr:
            switch (tone) {
            case TONE_SAC:   return VN_acr_sac;
            case TONE_HUYEN: return VN_acr_huyen;
            case TONE_HOI:   return VN_acr_hoi;
            case TONE_NGA:   return VN_acr_ngai;
            case TONE_NANG:  return VN_acr_nang;
            }
            return VN_acr;
        case VN_aci:
            switch (tone) {
            case TONE_SAC:   return VN_aci_sac;
            case TONE_HUYEN: return VN_aci_huyen;
            case TONE_HOI:   return VN_aci_hoi;
            case TONE_NGA:   return VN_aci_ngai;
            case TONE_NANG:  return VN_aci_nang;
            }
            return VN_aci;
        case VN_e:
            switch (tone) {
            case TONE_SAC:   return VN_e_sac;
            case TONE_HUYEN: return VN_e_huyen;
            case TONE_HOI:   return VN_e_hoi;
            case TONE_NGA:   return VN_e_ngai;
            case TONE_NANG:  return VN_e_nang;
            }
            return VN_e;
        case VN_eci:
            switch (tone) {
            case TONE_SAC:   return VN_eci_sac;
            case TONE_HUYEN: return VN_eci_huyen;
            case TONE_HOI:   return VN_eci_hoi;
            case TONE_NGA:   return VN_eci_ngai;
            case TONE_NANG:  return VN_eci_nang;
            }
            return VN_eci;
        case VN_o:
            switch (tone) {
            case TONE_SAC:   return VN_o_sac;
            case TONE_HUYEN: return VN_o_huyen;
            case TONE_HOI:   return VN_o_hoi;
            case TONE_NGA:   return VN_o_ngai;
            case TONE_NANG:  return VN_o_nang;
            }
            return VN_o;
        case VN_oci:
            switch (tone) {
            case TONE_SAC:   return VN_oci_sac;
            case TONE_HUYEN: return VN_oci_huyen;
            case TONE_HOI:   return VN_oci_hoi;
            case TONE_NGA:   return VN_oci_ngai;
            case TONE_NANG:  return VN_oci_nang;
            }
            return VN_oci;
        case VN_ohr:
            switch (tone) {
            case TONE_SAC:   return VN_ohr_sac;
            case TONE_HUYEN: return VN_ohr_huyen;
            case TONE_HOI:   return VN_ohr_hoi;
            case TONE_NGA:   return VN_ohr_ngai;
            case TONE_NANG:  return VN_ohr_nang;
            }
            return VN_ohr;
        case VN_u:
            switch (tone) {
            case TONE_SAC:   return VN_u_sac;
            case TONE_HUYEN: return VN_u_huyen;
            case TONE_HOI:   return VN_u_hoi;
            case TONE_NGA:   return VN_u_ngai;
            case TONE_NANG:  return VN_u_nang;
            }
            return VN_u;
        case VN_uhr:
            switch (tone) {
            case TONE_SAC:   return VN_uhr_sac;
            case TONE_HUYEN: return VN_uhr_huyen;
            case TONE_HOI:   return VN_uhr_hoi;
            case TONE_NGA:   return VN_uhr_ngai;
            case TONE_NANG:  return VN_uhr_nang;
            }
            return VN_uhr;
        case VN_i:
            switch (tone) {
            case TONE_SAC:   return VN_i_sac;
            case TONE_HUYEN: return VN_i_huyen;
            case TONE_HOI:   return VN_i_hoi;
            case TONE_NGA:   return VN_i_ngai;
            case TONE_NANG:  return VN_i_nang;
            }
            return VN_i;
        case VN_y:
            switch (tone) {
            case TONE_SAC:   return VN_y_sac;
            case TONE_HUYEN: return VN_y_huyen;
            case TONE_HOI:   return VN_y_hoi;
            case TONE_NGA:   return VN_y_ngai;
            case TONE_NANG:  return VN_y_nang;
            }
            return VN_y;
        }
        return base;
    }
}

/* Get circumflex version if applicable (a->â, e->ê, o->ô) */
static vn_vowel_t circumflex_of(vn_vowel_t vt)
{
    switch (vt) {
    case VH_A: return VH_ACI;
    case VH_E: return VH_ECI;
    case VH_O: return VH_OCI;
    default:   return VH_NONE;
    }
}

/* Get horn/breve version with 'w' (a->ă, o->ơ, u->ư) */
static vn_vowel_t horn_of(vn_vowel_t vt)
{
    switch (vt) {
    case VH_A: return VH_ACR;
    case VH_O: return VH_OHR;
    case VH_U: return VH_UHR;
    default:   return VH_NONE;
    }
}

static void result_init(telex_result_t *r)
{
    r->action = ACT_NONE;
    r->output_len = 0;
    r->backspace_count = 0;
}

static void result_add(telex_result_t *r, uint32_t cp)
{
    if (r->output_len < TELEX_MAX_OUTPUT)
        r->output[r->output_len++] = cp;
}

void telex_init(telex_ctx_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->enabled = true;
}

void telex_reset(telex_ctx_t *ctx)
{
    bool e = ctx->enabled;
    telex_init(ctx);
    ctx->enabled = e;
}

void telex_set_enabled(telex_ctx_t *ctx, bool enabled)
{
    ctx->enabled = enabled;
    if (!enabled) telex_reset(ctx);
}

void telex_reset_tracking(telex_ctx_t *ctx)
{
    ctx->word_len = 0;
    ctx->rendered_len = 0;
    ctx->shape_cancelled = false;
    ctx->shape_cancelled_len = 0;
    ctx->undo_valid = false;
    ctx->deleted_token_valid = false;
    ctx->boundary_count = 0;
}

void telex_commit_word(telex_ctx_t *ctx)
{
    if (ctx->word_len > 0) {
        if (ctx->boundary_count < TELEX_MAX_BOUNDARIES) {
            int n = ctx->boundary_count;
            memcpy(ctx->boundary_words[n], ctx->word, sizeof(ctx->word[0]) * ctx->word_len);
            ctx->boundary_lens[n] = ctx->word_len;
            ctx->boundary_count++;
        } else {
            memmove(ctx->boundary_words, ctx->boundary_words + 1,
                    (TELEX_MAX_BOUNDARIES - 1) * sizeof(ctx->boundary_words[0]));
            memmove(ctx->boundary_lens, ctx->boundary_lens + 1,
                    (TELEX_MAX_BOUNDARIES - 1) * sizeof(ctx->boundary_lens[0]));
            int n = TELEX_MAX_BOUNDARIES - 1;
            memcpy(ctx->boundary_words[n], ctx->word, sizeof(ctx->word[0]) * ctx->word_len);
            ctx->boundary_lens[n] = ctx->word_len;
        }
    }
    ctx->word_len = 0;
    ctx->rendered_len = 0;
    ctx->shape_cancelled = false;
    ctx->shape_cancelled_len = 0;
    ctx->undo_valid = false;
    ctx->deleted_token_valid = false;
}

static uint32_t token_cp(const telex_token_t *t);

void telex_handle_backspace(telex_ctx_t *ctx)
{
    if (ctx->word_len > 0) {
        ctx->deleted_token = ctx->word[ctx->word_len - 1];
        ctx->deleted_token_valid = true;
        ctx->word_len--;
        ctx->rendered_len = ctx->word_len;
        for (int i = 0; i < ctx->word_len; i++) {
            ctx->rendered_cps[i] = token_cp(&ctx->word[i]);
        }
        ctx->undo_valid = false;
        if (ctx->word_len == 0 || (ctx->shape_cancelled && ctx->word_len < ctx->shape_cancelled_len)) {
            ctx->shape_cancelled = false;
            ctx->shape_cancelled_len = 0;
        }
    } else if (ctx->word_len == 0 && ctx->boundary_count > 0) {
        /* Backspacing across the delimiter: restore the previous word into active buffer */
        int n = --ctx->boundary_count;
        memcpy(ctx->word, ctx->boundary_words[n], sizeof(ctx->word[0]) * ctx->boundary_lens[n]);
        ctx->word_len = ctx->boundary_lens[n];
        ctx->rendered_len = ctx->word_len;
        for (int i = 0; i < ctx->word_len; i++) {
            ctx->rendered_cps[i] = token_cp(&ctx->word[i]);
        }
        ctx->shape_cancelled = false;
        ctx->shape_cancelled_len = 0;
        ctx->undo_valid = false;
        ctx->deleted_token_valid = false;
    }
}

void telex_undo_last(telex_ctx_t *ctx)
{
    if (!ctx->undo_valid) return;
    memcpy(ctx->word, ctx->undo_word, sizeof(ctx->word));
    ctx->word_len = ctx->undo_word_len;
    ctx->rendered_len = ctx->undo_rendered_len;
    memcpy(ctx->rendered_cps, ctx->undo_rendered_cps, sizeof(ctx->rendered_cps));
    ctx->undo_valid = false;
}

static int token_is_vowel(const telex_token_t *t)
{
    return t->vowel_type != VH_NONE;
}

static uint32_t token_cp(const telex_token_t *t)
{
    if (token_is_vowel(t)) {
        uint32_t base = vowel_base_cp(t->vowel_type, t->is_upper);
        return apply_tone(base, t->tone, t->is_upper);
    }
    if (t->literal == VN_dd) {
        return t->is_upper ? VN_DD : VN_dd;
    }
    return t->literal;
}

static telex_result_t retype_word(telex_ctx_t *ctx)
{
    telex_result_t r;
    result_init(&r);

    uint32_t new_cps[TELEX_MAX_WORD];
    int new_len = ctx->word_len;
    for (int i = 0; i < new_len; i++) {
        new_cps[i] = token_cp(&ctx->word[i]);
    }

    /* Find longest common prefix with current screen representation (diff-based backspace) */
    int prefix = 0;
    while (prefix < ctx->rendered_len && prefix < new_len &&
           ctx->rendered_cps[prefix] == new_cps[prefix]) {
        prefix++;
    }

    r.action = ACT_BKSP_OUTPUT;
    r.backspace_count = ctx->rendered_len - prefix;
    for (int i = prefix; i < new_len; i++) {
        result_add(&r, new_cps[i]);
    }

    /* Update rendered state */
    ctx->rendered_len = new_len;
    for (int i = 0; i < new_len; i++) {
        ctx->rendered_cps[i] = new_cps[i];
    }

    return r;
}

static int choose_tone_vowel(const telex_ctx_t *ctx)
{
    int start = -1, end = -1;
    int limit = ctx->word_len;
    if (limit == 0) return -1;

    /* Find contiguous vowel nucleus within the current syllable */
    for (int i = limit - 1; i >= 0; i--) {
        if (!token_is_vowel(&ctx->word[i])) {
            if (end >= 0) break;
            continue;
        }
        if (end < 0) end = i;
        start = i;
    }
    if (start < 0) return -1;

    int count = end - start + 1;
    if (count == 1) return start;
    if (count >= 3) {
        /* In uyê/uyện and similar nuclei, y is an onset glide; tone on final vowel */
        if (ctx->word[start + 1].vowel_type == VH_Y)
            return end;
        return start + 1; /* iêu, uôi, ươi */
    }

    /* 2 vowels */
    int closed = (end + 1 < ctx->word_len && !token_is_vowel(&ctx->word[end + 1]));
    vn_vowel_t a = ctx->word[start].vowel_type;
    vn_vowel_t b = ctx->word[end].vowel_type;

    /* qu+a / qu+ă / qu+e / qu+o */
    if (a == VH_U && (b == VH_A || b == VH_ACR || b == VH_E || b == VH_ECI || b == VH_O || b == VH_OCI || b == VH_OHR) &&
        start > 0 && ctx->word[start - 1].vowel_type == VH_NONE &&
        (ctx->word[start - 1].literal == 'q' || ctx->word[start - 1].literal == 'Q'))
        return end;

    /* gi+o / gi+ơ / gi+u / gi+ư */
    if (a == VH_I && (b == VH_O || b == VH_OCI || b == VH_OHR || b == VH_U || b == VH_UHR) &&
        start > 0 && ctx->word[start - 1].vowel_type == VH_NONE &&
        (ctx->word[start - 1].literal == 'g' || ctx->word[start - 1].literal == 'G'))
        return end;

    /* Closed diphthongs: iê, uô, ươ */
    if (closed && ((a == VH_I && (b == VH_E || b == VH_ECI || b == VH_O || b == VH_OCI || b == VH_OHR)) ||
                   (a == VH_U && (b == VH_O || b == VH_OCI || b == VH_OHR)) ||
                   (a == VH_UHR && (b == VH_O || b == VH_OCI || b == VH_OHR))))
        return end;

    return start;
}

telex_result_t telex_process(telex_ctx_t *ctx, uint16_t keycode, bool pressed, bool is_upper)
{
    telex_result_t r;
    result_init(&r);
    if (!pressed) return r;
    if (!ctx->enabled) {
        r.action = ACT_OUTPUT;
        return r;
    }

    memcpy(ctx->undo_word, ctx->word, sizeof(ctx->word));
    ctx->undo_word_len = ctx->word_len;
    ctx->undo_rendered_len = ctx->rendered_len;
    memcpy(ctx->undo_rendered_cps, ctx->rendered_cps, sizeof(ctx->rendered_cps));
    ctx->undo_valid = true;

    char ch = keycode_to_char(keycode);
    if (!ch) {
        telex_commit_word(ctx);
        return r;
    }

    char typed_char = is_upper ? (ch >= 'a' && ch <= 'z' ? (char)(ch - 32) : ch) : ch;

    /* Once shape/tone is cancelled in this word, keep following keys literal until word boundary */
    if (ctx->shape_cancelled) {
        telex_token_t literal = {
            .literal = (uint32_t)typed_char,
            .vowel_type = VH_NONE,
            .tone = TONE_NONE,
            .is_upper = is_upper
        };
        if (ctx->word_len < TELEX_MAX_WORD)
            ctx->word[ctx->word_len++] = literal;
        r.action = ACT_OUTPUT;
        result_add(&r, (uint32_t)typed_char);
        if (ctx->rendered_len < TELEX_MAX_WORD) {
            ctx->rendered_cps[ctx->rendered_len++] = (uint32_t)typed_char;
        }
        return r;
    }

    /* Tone keys (s, f, r, x, j) */
    if (keycode == KEY_S || keycode == KEY_F || keycode == KEY_R ||
        keycode == KEY_X || keycode == KEY_J) {
        /* Accept hieues -> hiếu */
        if (ctx->word_len >= 3 &&
            ctx->word[ctx->word_len - 3].vowel_type == VH_E &&
            ctx->word[ctx->word_len - 2].vowel_type == VH_U &&
            ctx->word[ctx->word_len - 1].vowel_type == VH_E) {
            ctx->word_len--;
        }
        int idx = choose_tone_vowel(ctx);
        if (idx >= 0) {
            int tone = keycode == KEY_S ? TONE_SAC : keycode == KEY_F ? TONE_HUYEN :
                       keycode == KEY_R ? TONE_HOI : keycode == KEY_X ? TONE_NGA : TONE_NANG;
            if (ctx->word[idx].tone == tone) {
                /* Repeating tone key cancels tone and adds literal at cursor (e.g. á + s -> as) */
                ctx->word[idx].tone = TONE_NONE;
                if (ctx->word_len < TELEX_MAX_WORD) {
                    telex_token_t literal = {
                        .literal = (uint32_t)typed_char,
                        .vowel_type = VH_NONE,
                        .tone = TONE_NONE,
                        .is_upper = is_upper
                    };
                    ctx->word[ctx->word_len++] = literal;
                }
                ctx->shape_cancelled = true;
                ctx->shape_cancelled_len = ctx->word_len;
            } else {
                ctx->word[idx].tone = tone;
            }
            return retype_word(ctx);
        }
    }

    /* Horn/Breve marks (w, z) */
    if (keycode == KEY_W || keycode == KEY_Z) {
        if (keycode == KEY_W) {
            /* 1. Repeating 'w' on existing horned vowels cancels horn and restores plain vowel + 'w' */
            int has_horned = 0;
            for (int i = ctx->word_len - 1; i >= 0; i--) {
                if (ctx->word[i].vowel_type == VH_ACR || ctx->word[i].vowel_type == VH_OHR || ctx->word[i].vowel_type == VH_UHR) {
                    has_horned = 1;
                    break;
                }
            }
            if (has_horned) {
                for (int i = 0; i < ctx->word_len; i++) {
                    if (ctx->word[i].vowel_type == VH_ACR) ctx->word[i].vowel_type = VH_A;
                    else if (ctx->word[i].vowel_type == VH_OHR) ctx->word[i].vowel_type = VH_O;
                    else if (ctx->word[i].vowel_type == VH_UHR) ctx->word[i].vowel_type = VH_U;
                }
                if (ctx->word_len < TELEX_MAX_WORD) {
                    telex_token_t literal = {
                        .literal = (uint32_t)typed_char,
                        .vowel_type = VH_NONE,
                        .tone = TONE_NONE,
                        .is_upper = is_upper
                    };
                    ctx->word[ctx->word_len++] = literal;
                }
                ctx->shape_cancelled = true;
                ctx->shape_cancelled_len = ctx->word_len;
                return retype_word(ctx);
            }

            /* 2. uo + w -> ươ */
            int oi = -1, ui = -1;
            for (int i = ctx->word_len - 1; i >= 0; i--) {
                if (ctx->word[i].vowel_type == VH_O && oi < 0) oi = i;
                else if (ctx->word[i].vowel_type == VH_U && oi >= 0) { ui = i; break; }
            }
            if (ui >= 0 && oi > ui) {
                ctx->word[ui].vowel_type = VH_UHR;
                ctx->word[oi].vowel_type = VH_OHR;
                return retype_word(ctx);
            }

            /* 4. ua + w -> ưa */
            int ai = -1; ui = -1;
            for (int i = ctx->word_len - 1; i >= 0; i--) {
                if (ctx->word[i].vowel_type == VH_A && ai < 0) ai = i;
                else if (ctx->word[i].vowel_type == VH_U && ai >= 0) { ui = i; break; }
            }
            if (ui >= 0 && ai > ui) {
                ctx->word[ui].vowel_type = VH_UHR;
                return retype_word(ctx);
            }
        }
        for (int i = ctx->word_len - 1; i >= 0; i--) {
            if (!token_is_vowel(&ctx->word[i])) continue;
            vn_vowel_t nv = horn_of(ctx->word[i].vowel_type);
            if (nv != VH_NONE) {
                ctx->word[i].vowel_type = nv;
                return retype_word(ctx);
            }
        }
    }

    /* dd -> đ */
    if (keycode == KEY_D) {
        /* Repeating 'd' on existing 'đ' cancels and gives 'dd' */
        for (int i = ctx->word_len - 1; i >= 0; i--) {
            if (ctx->word[i].vowel_type == VH_NONE && ctx->word[i].literal == VN_dd) {
                ctx->word[i].literal = ctx->word[i].is_upper ? 'D' : 'd';
                if (ctx->word_len < TELEX_MAX_WORD) {
                    telex_token_t lit = {
                        .literal = (uint32_t)typed_char,
                        .vowel_type = VH_NONE,
                        .tone = TONE_NONE,
                        .is_upper = is_upper
                    };
                    ctx->word[ctx->word_len++] = lit;
                }
                ctx->shape_cancelled = true;
                ctx->shape_cancelled_len = ctx->word_len;
                return retype_word(ctx);
            }
        }
        if (ctx->word_len > 0 &&
            (ctx->word[ctx->word_len - 1].literal == 'd' || ctx->word[ctx->word_len - 1].literal == 'D') &&
            !token_is_vowel(&ctx->word[ctx->word_len - 1])) {
            ctx->word[ctx->word_len - 1].literal = VN_dd;
            return retype_word(ctx);
        }
        /* Trailing d (e.g. dood -> đô) */
        for (int i = ctx->word_len - 1; i >= 0; i--) {
            if (ctx->word[i].vowel_type == VH_NONE &&
                (ctx->word[i].literal == 'd' || ctx->word[i].literal == 'D')) {
                ctx->word[i].literal = VN_dd;
                return retype_word(ctx);
            }
        }
    }

    /* Vowels and Consonants */
    telex_token_t t = {
        .literal = (uint32_t)typed_char,
        .vowel_type = kc_to_vowel_type(keycode),
        .tone = TONE_NONE,
        .is_upper = is_upper
    };

    if (ctx->word_len < TELEX_MAX_WORD) {
        /* Repeating shape key (aa->â, â+a->aa; ee->ê, ê+e->ee; oo->ô, ô+o->oo) */
        if (keycode == KEY_A || keycode == KEY_E || keycode == KEY_O) {
            vn_vowel_t marked = keycode == KEY_A ? VH_ACI :
                                keycode == KEY_E ? VH_ECI : VH_OCI;
            vn_vowel_t plain = keycode == KEY_A ? VH_A :
                               keycode == KEY_E ? VH_E : VH_O;
            for (int i = ctx->word_len - 1; i >= 0; i--) {
                if (ctx->word[i].vowel_type == marked) {
                    ctx->word[i].vowel_type = plain;
                    if (ctx->word_len < TELEX_MAX_WORD) {
                        telex_token_t literal = {
                            .literal = (uint32_t)typed_char,
                            .vowel_type = plain,
                            .tone = TONE_NONE,
                            .is_upper = is_upper
                        };
                        ctx->word[ctx->word_len++] = literal;
                    }
                    ctx->shape_cancelled = true;
                    ctx->shape_cancelled_len = ctx->word_len;
                    return retype_word(ctx);
                }
            }
        }

        /* Structure mark after coda (e.g. tete -> têt, then tetes -> tết) */
        if (keycode == KEY_A || keycode == KEY_E || keycode == KEY_O) {
            vn_vowel_t base = kc_to_vowel_type(keycode);
            int prior = -1, has_coda = 0;
            for (int i = ctx->word_len - 1; i >= 0; i--) {
                if (ctx->word[i].vowel_type == base) { prior = i; break; }
                if (!token_is_vowel(&ctx->word[i])) has_coda = 1;
            }
            if (prior >= 0 && has_coda) {
                vn_vowel_t nv = circumflex_of(base);
                if (nv != VH_NONE) {
                    ctx->word[prior].vowel_type = nv;
                    return retype_word(ctx);
                }
            }
        }

        /* uw followed by o -> ươ */
        if (keycode == KEY_O && ctx->word_len > 0 &&
            ctx->word[ctx->word_len - 1].vowel_type == VH_UHR) {
            t.vowel_type = VH_OHR;
        }

        /* Flexible circumflex across glide: toio -> tôi, dauas -> dấu */
        if (keycode == KEY_A || keycode == KEY_E || keycode == KEY_O) {
            vn_vowel_t base = kc_to_vowel_type(keycode);
            int prior_vowel = -1;
            for (int i = ctx->word_len - 2; i >= 0; i--) {
                if (ctx->word[i].vowel_type == base) { prior_vowel = i; break; }
            }
            if (prior_vowel >= 0) {
                int has_intervening_vowel = 0;
                for (int i = prior_vowel + 1; i < ctx->word_len; i++) {
                    if (token_is_vowel(&ctx->word[i])) { has_intervening_vowel = 1; break; }
                }
                if (has_intervening_vowel) {
                    vn_vowel_t nv = circumflex_of(base);
                    if (nv != VH_NONE) {
                        ctx->word[prior_vowel].vowel_type = nv;
                        return retype_word(ctx);
                    }
                }
            }
        }

        /* hieue -> hiêu */
        if (t.vowel_type == VH_E && ctx->word_len >= 2 &&
            ctx->word[ctx->word_len - 2].vowel_type == VH_E &&
            ctx->word[ctx->word_len - 1].vowel_type == VH_U) {
            ctx->word[ctx->word_len - 2].vowel_type = VH_ECI;
            return retype_word(ctx);
        }

        /* aa/ee/oo -> â/ê/ô */
        if (token_is_vowel(&t) && ctx->word_len > 0 &&
            ctx->word[ctx->word_len - 1].vowel_type == t.vowel_type) {
            vn_vowel_t nv = circumflex_of(t.vowel_type);
            int after_ao = (t.vowel_type == VH_O && ctx->word_len >= 2 &&
                            ctx->word[ctx->word_len - 2].vowel_type == VH_A);
            if (nv != VH_NONE && !after_ao) {
                ctx->word[ctx->word_len - 1].vowel_type = nv;
                return retype_word(ctx);
            }
        }

        ctx->word[ctx->word_len++] = t;

        /* Coda tone re-evaluation within current syllable */
        if (!token_is_vowel(&t)) {
            int old = -1;
            for (int i = 0; i < ctx->word_len; i++) {
                if (ctx->word[i].tone != TONE_NONE) { old = i; break; }
            }
            if (old >= 0) {
                int tone = ctx->word[old].tone;
                int target = choose_tone_vowel(ctx);
                if (target >= 0 && target != old) {
                    for (int i = 0; i < ctx->word_len; i++) ctx->word[i].tone = TONE_NONE;
                    ctx->word[target].tone = tone;
                    return retype_word(ctx);
                }
            }
        }
    }

    r.action = ACT_OUTPUT;
    uint32_t out_cp = token_cp(&t);
    result_add(&r, out_cp);
    if (ctx->rendered_len < TELEX_MAX_WORD) {
        ctx->rendered_cps[ctx->rendered_len++] = out_cp;
    }
    return r;
}

