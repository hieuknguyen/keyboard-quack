/* Dump the vn_dead_keys table as "codepoint tone-key mark-key dot-key base-key"
 * so it can be validated against the X11 Compose file. */
#include <stdio.h>
#include <stdint.h>
#include <linux/input.h>

typedef struct {
    uint32_t cp;
    uint8_t  tone;
    uint8_t  mark;
    uint8_t  dot;
    uint8_t  base;
    uint8_t  direct;
} vn_dead_key_t;

static const vn_dead_key_t vn_dead_keys[] = {
    { 0x00E1, KEY_S, 0, 0, KEY_A, 0 },
    { 0x00E0, KEY_F, 0, 0, KEY_A, 0 },
    { 0x1EA3, KEY_R, 0, 0, KEY_A, 0 },
    { 0x00E3, KEY_X, 0, 0, KEY_A, 0 },
    { 0x1EA1, 0,     0, KEY_J, KEY_A, 0 },
    { 0x0103, 0,     KEY_W, 0, KEY_A, 0 },
    { 0x1EAF, KEY_S, KEY_W, 0, KEY_A, 0 },
    { 0x1EB1, KEY_F, KEY_W, 0, KEY_A, 0 },
    { 0x1EB3, KEY_R, KEY_W, 0, KEY_A, 0 },
    { 0x1EB5, KEY_X, KEY_W, 0, KEY_A, 0 },
    { 0x1EB7, 0,     KEY_W, KEY_J, KEY_A, 0 },
    { 0x00E2, 0,     KEY_O, 0, KEY_A, 0 },
    { 0x1EA5, KEY_S, KEY_O, 0, KEY_A, 0 },
    { 0x1EA7, KEY_F, KEY_O, 0, KEY_A, 0 },
    { 0x1EA9, KEY_R, KEY_O, 0, KEY_A, 0 },
    { 0x1EAB, KEY_X, KEY_O, 0, KEY_A, 0 },
    { 0x1EAD, 0,     KEY_O, KEY_J, KEY_A, 0 },
    { 0x00E9, KEY_S, 0, 0, KEY_E, 0 },
    { 0x00E8, KEY_F, 0, 0, KEY_E, 0 },
    { 0x1EBB, KEY_R, 0, 0, KEY_E, 0 },
    { 0x1EBD, KEY_X, 0, 0, KEY_E, 0 },
    { 0x1EB9, 0,     0, KEY_J, KEY_E, 0 },
    { 0x00EA, 0,     KEY_O, 0, KEY_E, 0 },
    { 0x1EBF, KEY_S, KEY_O, 0, KEY_E, 0 },
    { 0x1EC1, KEY_F, KEY_O, 0, KEY_E, 0 },
    { 0x1EC3, KEY_R, KEY_O, 0, KEY_E, 0 },
    { 0x1EC5, KEY_X, KEY_O, 0, KEY_E, 0 },
    { 0x1EC7, 0,     KEY_O, KEY_J, KEY_E, 0 },
    { 0x00ED, KEY_S, 0, 0, KEY_I, 0 },
    { 0x00EC, KEY_F, 0, 0, KEY_I, 0 },
    { 0x1EC9, KEY_R, 0, 0, KEY_I, 0 },
    { 0x0129, KEY_X, 0, 0, KEY_I, 0 },
    { 0x1ECB, 0,     0, KEY_J, KEY_I, 0 },
    { 0x00F3, KEY_S, 0, 0, KEY_O, 0 },
    { 0x00F2, KEY_F, 0, 0, KEY_O, 0 },
    { 0x1ECF, KEY_R, 0, 0, KEY_O, 0 },
    { 0x00F5, KEY_X, 0, 0, KEY_O, 0 },
    { 0x1ECD, 0,     0, KEY_J, KEY_O, 0 },
    { 0x00F4, 0,     KEY_O, 0, KEY_O, 0 },
    { 0x1ED1, KEY_S, KEY_O, 0, KEY_O, 0 },
    { 0x1ED3, KEY_F, KEY_O, 0, KEY_O, 0 },
    { 0x1ED5, KEY_R, KEY_O, 0, KEY_O, 0 },
    { 0x1ED7, KEY_X, KEY_O, 0, KEY_O, 0 },
    { 0x1ED9, 0,     KEY_O, KEY_J, KEY_O, 0 },
    { 0x01A1, 0,     KEY_Z, 0, KEY_O, 0 },
    { 0x1EDB, KEY_S, KEY_Z, 0, KEY_O, 0 },
    { 0x1EDD, KEY_F, KEY_Z, 0, KEY_O, 0 },
    { 0x1EDF, KEY_R, KEY_Z, 0, KEY_O, 0 },
    { 0x1EE1, KEY_X, KEY_Z, 0, KEY_O, 0 },
    { 0x1EE3, 0,     KEY_Z, KEY_J, KEY_O, 0 },
    { 0x00FA, KEY_S, 0, 0, KEY_U, 0 },
    { 0x00F9, KEY_F, 0, 0, KEY_U, 0 },
    { 0x1EE7, KEY_R, 0, 0, KEY_U, 0 },
    { 0x0169, KEY_X, 0, 0, KEY_U, 0 },
    { 0x1EE5, 0,     0, KEY_J, KEY_U, 0 },
    { 0x01B0, 0,     KEY_Z, 0, KEY_U, 0 },
    { 0x1EE9, KEY_S, KEY_Z, 0, KEY_U, 0 },
    { 0x1EEB, KEY_F, KEY_Z, 0, KEY_U, 0 },
    { 0x1EED, KEY_R, KEY_Z, 0, KEY_U, 0 },
    { 0x1EEF, KEY_X, KEY_Z, 0, KEY_U, 0 },
    { 0x1EF1, 0,     KEY_Z, KEY_J, KEY_U, 0 },
    { 0x00FD, KEY_S, 0, 0, KEY_Y, 0 },
    { 0x1EF3, KEY_F, 0, 0, KEY_Y, 0 },
    { 0x1EF7, KEY_R, 0, 0, KEY_Y, 0 },
    { 0x1EF9, KEY_X, 0, 0, KEY_Y, 0 },
    { 0x1EF5, 0,     0, KEY_J, KEY_Y, 0 },
    { 0x0111, 0, 0, 0, 0, KEY_D },
};

static const char *kname(uint8_t k)
{
    static char buf[8];
    switch (k) {
    case KEY_S: return "s"; case KEY_F: return "f"; case KEY_R: return "r";
    case KEY_X: return "x"; case KEY_J: return "j"; case KEY_W: return "w";
    case KEY_Z: return "z"; case KEY_O: return "o"; case KEY_D: return "d";
    case KEY_A: return "a"; case KEY_E: return "e"; case KEY_I: return "i";
    case KEY_U: return "u"; case KEY_Y: return "y";
    default: snprintf(buf, sizeof(buf), "?%d", k); return buf;
    }
}

int main(void)
{
    for (size_t i = 0; i < sizeof(vn_dead_keys) / sizeof(vn_dead_keys[0]); i++) {
        const vn_dead_key_t *v = &vn_dead_keys[i];
        printf("U+%04X", v->cp);
        if (v->direct) {
            printf(" direct=%s\n", kname(v->direct));
            continue;
        }
        printf(" seq=");
        if (v->tone) printf("%s", kname(v->tone));
        if (v->mark) printf("%s", kname(v->mark));
        if (v->dot)  printf("%s", kname(v->dot));
        printf("%s\n", kname(v->base));
    }
    return 0;
}