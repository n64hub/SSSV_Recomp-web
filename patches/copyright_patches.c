#include "sssv_patch_common.h"
#include "sssv_render_context.h"

extern u8 D_80302E88[];

/* The HD copyright pack contains separate background and lettering textures.
 * Give its background a viewport-sized rectangle. RT64 maps a centre crop of
 * its 32:9 panorama into that rectangle, stretching only beyond 32:9, and keeps
 * the text pass at the native centred 320x240 layout. */
RECOMP_PATCH void draw_sprite(Gfx** dl, uSprite* image, u16 width, u16 height,
    u16 scale_x, u16 scale_y, u8 flip_x, u8 flip_y,
    u16 screen_x, u16 screen_y, u8 depth) {
    const s32 copyright = depth == 16 && width == 320 && height == 240 &&
        image == (uSprite*)(D_80302E88 + 0x74050) && recomp_get_hd_title_enabled();
    s32 draw_x_quarters = (s32)screen_x * 4;
    uSprite* sprite = &gDisplayListContext->sprites[gDisplayListContext->usedSprites];

    if (copyright) {
        const f32 aspect = recomp_get_target_aspect_ratio(SSSV_BASE_ASPECT);
        scale_x = (u16)MAX(320, (s32)(240.0f * aspect + 0.5f));
        draw_x_quarters = (320 - (s32)scale_x) * 2;
        rc_ensure_rt64_extended_enabled(dl);
        gEXSetRectAlign((*dl)++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER,
            -640, 0, -640, 0);
        gEXSetScissorAlign((*dl)++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_RIGHT,
            0, 0, -320, 0, 0, 0, 320, 240);
        gDPSetScissor((*dl)++, G_SC_NON_INTERLACE, 0, 0, 320, 240);
    }

    gDPPipeSync((*dl)++);
    switch (depth) {
    case 8:
        guSprite2DInit(sprite, image, 0, width, width, height, G_IM_FMT_I, G_IM_SIZ_8b, 0, 0);
        break;
    case 16:
        guSprite2DInit(sprite, image, 0, width, width, height, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0);
        break;
    }
    gSPSprite2DBase((*dl)++, OS_K0_TO_PHYSICAL(sprite));
    gSPSprite2DScaleFlip((*dl)++, (width / (f32)scale_x) * 1024.0f,
        (height / (f32)scale_y) * 1024.0f, flip_x, flip_y);
    gSPSprite2DDraw((*dl)++, draw_x_quarters, (s32)screen_y * 4);
    ++gDisplayListContext->usedSprites;

    if (copyright) {
        gDPPipeSync((*dl)++);
        gEXSetScissorAlign((*dl)++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER,
            -160, 0, -160, 0, 0, 0, 320, 240);
        gDPSetScissor((*dl)++, G_SC_NON_INTERLACE, 8, 8, 312, 232);
    }
}
