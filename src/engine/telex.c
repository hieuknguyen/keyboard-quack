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

static bool is_vowel_kc(uint16_t kc)
{
    return (kc == KEY_A || kc == KEY_E || kc == KEY_O ||
            kc == KEY_U || kc == KEY_I || kc == KEY_Y);
}

/* Base codepoint (no tone) for each vowel type */
static uint32_t vowel_base_cp(vn_vowel_t vt)
{
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
static uint32_t apply_tone(uint32_t base, int tone)
{
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

/* Set up a backspace+retype result: backspace suffix+1 chars after the vowel,
 * then output the (new) vowel followed by the suffix. Updates tracking. */
static telex_result_t make_retype(telex_ctx_t *ctx, uint32_t new_vowel_cp,
                                  vn_vowel_t new_vtype, int new_tone)
{
    telex_result_t r;
    result_init(&r);
    r.action = ACT_BKSP_OUTPUT;
    r.backspace_count = ctx->suffix_count + 1;

    result_add(&r, new_vowel_cp);
    for (int i = 0; i < ctx->suffix_count; i++)
        result_add(&r, ctx->suffix[i]);

    /* Update tracking */
    ctx->last_vowel_base = new_vowel_cp;
    ctx->last_vowel_type = new_vtype;
    ctx->last_tone = new_tone;
    return r;
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
    ctx->last_vowel_base = 0;
    ctx->last_vowel_type = VH_NONE;
    ctx->last_tone = TONE_NONE;
    ctx->suffix_count = 0;
    ctx->last_was_d = false;
    ctx->word_len = 0;
    ctx->rendered_len = 0;
    ctx->shape_cancelled = false;
}

void telex_commit_boundary(telex_ctx_t *ctx)
{
    ctx->saved_word_len = ctx->word_len;
    ctx->saved_rendered_len = ctx->rendered_len;
    memcpy(ctx->saved_word, ctx->word, sizeof(ctx->word));
    ctx->boundary_saved = (ctx->word_len > 0);
    telex_reset_tracking(ctx);
}

void telex_restore_boundary(telex_ctx_t *ctx)
{
    if (!ctx->boundary_saved || ctx->word_len != 0) return;
    memcpy(ctx->word, ctx->saved_word, sizeof(ctx->word));
    ctx->word_len = ctx->saved_word_len;
    ctx->rendered_len = ctx->saved_rendered_len;
    ctx->boundary_saved = false;
}

void telex_undo_last(telex_ctx_t *ctx)
{
    if (!ctx->undo_valid) return;
    memcpy(ctx->word, ctx->undo_word, sizeof(ctx->word));
    ctx->word_len = ctx->undo_word_len;
    ctx->rendered_len = ctx->undo_rendered_len;
    ctx->undo_valid = false;
}

static int token_is_vowel(const telex_token_t *t)
{
    return t->vowel_type != VH_NONE;
}

static uint32_t token_cp(const telex_token_t *t)
{
    return token_is_vowel(t) ? apply_tone(vowel_base_cp(t->vowel_type), t->tone)
                             : t->literal;
}

static telex_result_t retype_word(telex_ctx_t *ctx)
{
    telex_result_t r;
    result_init(&r);
    r.action = ACT_BKSP_OUTPUT;
    r.backspace_count = ctx->rendered_len;
    for (int i = 0; i < ctx->word_len; i++) result_add(&r, token_cp(&ctx->word[i]));
    ctx->rendered_len = r.output_len;
    return r;
}

static int choose_tone_vowel(const telex_ctx_t *ctx)
{
    int start = -1, end = -1;
    /* Find the final contiguous vowel nucleus (the letters immediately
     * before the current coda/consonant). */
    for (int i = ctx->word_len - 1; i >= 0; i--) {
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
        /* In uyê/uyệ and similar nuclei, y is a glide, so the tone belongs
         * to the final e (duyệt, quyết, Nguyễn), not to y. */
        if (ctx->word[start + 1].vowel_type == VH_Y)
            return end;
        return start + 1; /* iêu, uôi, ươi */
    }

    /* A closed iê/uô/ươ nucleus puts the tone on its second vowel:
     * tiếng, uống, mượn. Open diphthongs (mía, múa, áo, hòa) use the first. */
    int closed = (end + 1 < ctx->word_len &&
                  !token_is_vowel(&ctx->word[end + 1]));
    vn_vowel_t a = ctx->word[start].vowel_type;
    vn_vowel_t b = ctx->word[end].vowel_type;
    if (a == VH_I && (b == VH_O || b == VH_OCI || b == VH_OHR ||
                      b == VH_U || b == VH_UHR))
        return end; /* iô/iơ/iư: giò, giờ, giữ, giống */
    if (closed && ((a == VH_I && (b == VH_E || b == VH_ECI ||
                                  b == VH_O || b == VH_OCI || b == VH_OHR)) ||
                   (a == VH_U && (b == VH_O || b == VH_OCI || b == VH_OHR)) ||
                   (a == VH_UHR && (b == VH_O || b == VH_OCI || b == VH_OHR))))
        return end;
    return start;
}

telex_result_t telex_process(telex_ctx_t *ctx, uint16_t keycode, bool pressed)
{
    telex_result_t r; result_init(&r);
    if (!pressed) return r;
    if (!ctx->enabled) { r.action = ACT_OUTPUT; return r; }
    if (ctx->word_len == 0) ctx->boundary_saved = false;
    memcpy(ctx->undo_word, ctx->word, sizeof(ctx->word));
    ctx->undo_word_len = ctx->word_len;
    ctx->undo_rendered_len = ctx->rendered_len;
    ctx->undo_valid = true;
    char ch = keycode_to_char(keycode);
    if (!ch) { telex_reset_tracking(ctx); return r; }

    /* Tone keys modify the nucleus of the current vowel cluster. */
    if (!ctx->shape_cancelled &&
        (keycode == KEY_S || keycode == KEY_F || keycode == KEY_R ||
        keycode == KEY_X || keycode == KEY_J)) {
        /* Accept the commonly mistyped spelling hieues/hieuf/... for
         * hiếu/hiểu/...: the final e is a redundant tone carrier after ieu. */
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
                /* Repeating the same tone key cancels the tone and keeps the
                 * key as a literal at the cursor (á+s -> as). */
                ctx->word[idx].tone = TONE_NONE;
                if (ctx->word_len < TELEX_MAX_WORD) {
                    telex_token_t literal = { .literal = (uint32_t)ch,
                                              .vowel_type = VH_NONE,
                                              .tone = TONE_NONE };
                    ctx->word[ctx->word_len++] = literal;
                }
                /* Keep tone processing disabled for the remainder of this
                 * word: the repeated key is now literal (hiéu+s -> hieus),
                 * so later letters must not reapply a tone to the old vowel. */
                ctx->shape_cancelled = true;
            } else {
                ctx->word[idx].tone = tone;
            }
            return retype_word(ctx);
        }
    }

    /* w/o/z marks apply to the most recent compatible vowel. */
    if (keycode == KEY_W || keycode == KEY_Z) {
        /* Accept uo+w in any order within the current syllable, including
         * after a coda: muonw, muow, muojw -> mươ... */
        if (keycode == KEY_W) {
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
            /* Likewise, uaw is a flexible spelling of ưa: w belongs to
             * the u, not the following a. */
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
            vn_vowel_t nv = VH_NONE;
            if (keycode == KEY_W) nv = horn_of(ctx->word[i].vowel_type);
            else if (keycode == KEY_Z) nv = horn_of(ctx->word[i].vowel_type);
            if (nv != VH_NONE) {
                ctx->word[i].vowel_type = nv;
                return retype_word(ctx);
            }
        }
    }

    /* dd -> đ */
    if (keycode == KEY_D) {
        for (int i = ctx->word_len - 1; i >= 0; i--) {
            if (ctx->word[i].vowel_type == VH_NONE &&
                ctx->word[i].literal == VN_dd) {
                ctx->word[i].literal = 'd';
                if (ctx->word_len < TELEX_MAX_WORD) {
                    int pos = (i + 1 < ctx->word_len) ? ctx->word_len : i + 1;
                    if (pos == i + 1) {
                        ctx->word[i + 1].literal = 'd';
                        ctx->word[i + 1].vowel_type = VH_NONE;
                        ctx->word[i + 1].tone = TONE_NONE;
                    } else {
                        ctx->word[pos].literal = 'd';
                        ctx->word[pos].vowel_type = VH_NONE;
                        ctx->word[pos].tone = TONE_NONE;
                    }
                    ctx->word_len++;
                }
                return retype_word(ctx);
            }
        }
    }
    if (keycode == KEY_D && ctx->word_len > 0 &&
        ctx->word[ctx->word_len - 1].literal == 'd' &&
        !token_is_vowel(&ctx->word[ctx->word_len - 1])) {
        ctx->word[ctx->word_len - 1].literal = VN_dd;
        return retype_word(ctx);
    }
    /* A trailing d can also act as the đ marker after the rest of the
     * syllable has already been typed (dood -> đô with the d converted). */
    if (keycode == KEY_D) {
        for (int i = ctx->word_len - 1; i >= 0; i--) {
            if (ctx->word[i].vowel_type == VH_NONE && ctx->word[i].literal == 'd') {
                ctx->word[i].literal = VN_dd;
                return retype_word(ctx);
            }
        }
    }

    telex_token_t t = { .literal = (uint32_t)ch, .vowel_type = kc_to_vowel_type(keycode), .tone = TONE_NONE };
    if (ctx->word_len < TELEX_MAX_WORD) {
        /* Repeating the shape key removes that shape at the same position. */
        if (keycode == KEY_A || keycode == KEY_E || keycode == KEY_O) {
            vn_vowel_t marked = keycode == KEY_A ? VH_ACI :
                                keycode == KEY_E ? VH_ECI : VH_OCI;
            vn_vowel_t plain = keycode == KEY_A ? VH_A :
                               keycode == KEY_E ? VH_E : VH_O;
            for (int i = ctx->word_len - 1; i >= 0; i--) {
                if (ctx->word[i].vowel_type == marked) {
                    ctx->word[i].vowel_type = plain;
                    if (ctx->word_len < TELEX_MAX_WORD) {
                        int pos = (i + 1 < ctx->word_len) ? ctx->word_len : i + 1;
                        ctx->word[pos].literal = vowel_base_cp(plain);
                        ctx->word[pos].vowel_type = plain;
                        ctx->word[pos].tone = TONE_NONE;
                        ctx->word_len++;
                    }
                    ctx->shape_cancelled = true;
                    return retype_word(ctx);
                }
            }
        }
        /* Allow a structure mark after the coda: tete -> têt, then tetes
         * -> tết.  Require a consonant between the two vowels so ordinary
         * diphthongs such as ao/eo are not rewritten accidentally. */
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
        /* uw followed by o is the other natural spelling of ươ. */
        if (keycode == KEY_O && ctx->word_len > 0 &&
            ctx->word[ctx->word_len - 1].vowel_type == VH_UHR)
            t.vowel_type = VH_OHR;
        /* Flexible circumflex spelling across a glide: toio -> tôi,
         * dauas -> dấu, and analogous e-clusters. */
        if (keycode == KEY_A || keycode == KEY_E || keycode == KEY_O) {
            vn_vowel_t base = kc_to_vowel_type(keycode);
            int prior_vowel = -1;
            for (int i = ctx->word_len - 2; i >= 0; i--) {
                if (ctx->word[i].vowel_type == base) { prior_vowel = i; break; }
            }
            if (prior_vowel >= 0) {
                int has_intervening_vowel = 0;
                for (int i = prior_vowel + 1; i < ctx->word_len; i++)
                    if (token_is_vowel(&ctx->word[i])) { has_intervening_vowel = 1; break; }
                if (has_intervening_vowel) {
                    vn_vowel_t nv = circumflex_of(base);
                    if (nv != VH_NONE) {
                        ctx->word[prior_vowel].vowel_type = nv;
                        return retype_word(ctx);
                    }
                }
            }
        }
        /* Telex spelling of iêu/hiêu uses a final e to form ê before u:
         * hieue -> hiêu, then a later tone key changes it to hiếu. */
        if (t.vowel_type == VH_E && ctx->word_len >= 2 &&
            ctx->word[ctx->word_len - 2].vowel_type == VH_E &&
            ctx->word[ctx->word_len - 1].vowel_type == VH_U) {
            ctx->word[ctx->word_len - 2].vowel_type = VH_ECI;
            return retype_word(ctx);
        }
        /* aa/ee/oo -> â/ê/ô; swallow the second key via retype. */
        if (token_is_vowel(&t) && ctx->word_len > 0 &&
            ctx->word[ctx->word_len - 1].vowel_type == t.vowel_type) {
            vn_vowel_t nv = circumflex_of(t.vowel_type);
            /* ao is already a complete diphthong: saoo means "saoo", not
             * the non-word "saô".  Keep the final o literal in that case. */
            int after_ao = (t.vowel_type == VH_O && ctx->word_len >= 2 &&
                            ctx->word[ctx->word_len - 2].vowel_type == VH_A);
            if (nv != VH_NONE && !after_ao) {
                ctx->word[ctx->word_len - 1].vowel_type = nv;
                return retype_word(ctx);
            }
        }
        ctx->word[ctx->word_len++] = t;

        /* A coda can change the orthographic nucleus (e.g. muowj is
         * temporarily mụơ, then muowjn must become mượn). Re-evaluate an
         * existing tone as soon as the coda arrives. */
        if (!token_is_vowel(&t)) {
            int old = -1;
            for (int i = 0; i < ctx->word_len; i++)
                if (ctx->word[i].tone != TONE_NONE) { old = i; break; }
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
    result_add(&r, token_cp(&t));
    ctx->rendered_len++;
    return r;
}
