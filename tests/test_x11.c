#include <stdio.h>
#include "inject/uinput_inject.h"

int main(void)
{
    inject_ctx_t ctx;
    inject_init(&ctx);

    printf("backend=%d\n", ctx.backend);
    printf("keycode=%d backspace_kc=%d\n", ctx.keycode, ctx.bspc_keycode);

    /* Test typing a Vietnamese char via XTest */
    if (ctx.backend == 1) {
        int r = inject_unicode(&ctx, 0x00E1);  /* á */
        printf("inject_unicode á -> %d\n", r);
        r = inject_unicode(&ctx, 0x1EA1);      /* ạ */
        printf("inject_unicode ạ -> %d\n", r);
        r = inject_unicode(&ctx, 0x0111);      /* đ */
        printf("inject_unicode đ -> %d\n", r);
    }

    inject_cleanup(&ctx);
    printf("done\n");
    return 0;
}