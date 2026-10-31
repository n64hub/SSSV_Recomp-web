#include "sssv_patch_common.h"
#include "sssv_render_context.h"

extern u8 gInitialisationState;

static f32 s_capture_aspect = 4.0f / 3.0f;

// Only the frame about to be copied for the pause preview needs a packed wide
// capture. Other native framebuffer users (including title tiles) keep their
// original layout. The display-list command is snapshotted per workload.
void sssv_prepare_pause_capture(void) {
    f32 ratio = 1.0f;
    // The game waits for the two buffered tasks before copying the last frame.
    // Both preparation frames need the same capture projection.
    if (gInitialisationState != 0) {
        s_capture_aspect = recomp_get_target_aspect_ratio(4.0f / 3.0f);
        ratio = s_capture_aspect / (4.0f / 3.0f);
    }
    rc_ensure_rt64_extended_enabled(&gMainDL);
    gEXSetRenderToRAMAspect(gMainDL++, ratio);
}

RECOMP_PATCH void func_803925D0_7A3C80(u16* src, u16* dst) {
    // Physical RAM rows are always 320 pixels, even when the virtual viewport
    // is wider. The original half-width capture is exactly 160 by 240.
    s32 y, x;
    for (y = 0; y < 240; ++y) {
        for (x = 0; x < 160; ++x) {
            dst[y * 160 + x] = src[y * 320 + x * 2];
        }
    }
}

RECOMP_PATCH void func_803962EC_7A799C(Gfx** dl, s16 x0, s16 y0,
    s16 x1, s16 y1, u8* img, u16 width, u16 height) {
    // This is the fixed framebuffer-copy buffer used by load_pause_menu.
    const s32 preview = img == (u8*)0x800C5A40;
    if (preview) {
        const f32 aspect = recomp_get_target_aspect_ratio(4.0f / 3.0f);
        const f32 wide_width = 240.0f * aspect;
        // The original animation supplies the same rectangle in both directions.
        // Blend its horizontal bounds from the full expanded viewport to the
        // native-aspect menu box. Fit the captured scene inside those bounds.
        f32 progress = (f32)x0 / 760.0f;
        f32 left, right, top, bottom, w, h, image_w, image_h;
        if (progress < 0.0f) progress = 0.0f;
        if (progress > 1.0f) progress = 1.0f;
        left = x0 * 0.25f - (wide_width - 320.0f) * 0.5f * (1.0f - progress);
        right = x1 * 0.25f + (wide_width - 320.0f) * 0.5f * (1.0f - progress);
        top = y0 * 0.25f;
        bottom = y1 * 0.25f;
        w = right - left;
        h = bottom - top;
        image_w = w;
        image_h = w / s_capture_aspect;
        if (image_h > h) {
            image_h = h;
            image_w = h * s_capture_aspect;
        }

        rc_ensure_rt64_extended_enabled(dl);
        gEXSetRectAlign((*dl)++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER,
            -640, 0, -640, 0);
        gEXSetScissorAlign((*dl)++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_RIGHT,
            0, 0, -320, 0, 0, 0, 320, 240);
        gDPSetScissor((*dl)++, G_SC_NON_INTERLACE, 0, 0, 320, 240);
        gDPPipeSync((*dl)++);
        gDPSetCombineMode((*dl)++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
        gDPSetPrimColor((*dl)++, 0, 0, 0, 0, 0, 255);
        // Keep encoded fill coordinates positive; the signed origin offset
        // carries the part of the expanded frame left of the native canvas.
        gEXSetRectAlign((*dl)++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER,
            (s16)(left * 4) - 640, 0, (s16)(left * 4) - 640, 0);
        gDPFillRectangle((*dl)++, 0, (s16)top, (s16)w, (s16)bottom);
        gEXSetRectAlign((*dl)++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER,
            -640, 0, -640, 0);
        gDPPipeSync((*dl)++);
        gDPSetPrimColor((*dl)++, 0, 0, 255, 255, 255, 255);
        gDPSetCombineLERP((*dl)++, PRIMITIVE, 0, TEXEL0, 0,
            0, 0, 0, PRIMITIVE, PRIMITIVE, 0, TEXEL0, 0, 0, 0, 0, PRIMITIVE);
        x0 = (s16)((left + (w - image_w) * 0.5f) * 4.0f);
        x1 = (s16)((left + (w + image_w) * 0.5f) * 4.0f);
        y0 = (s16)((top + (h - image_h) * 0.5f) * 4.0f);
        y1 = (s16)((top + (h + image_h) * 0.5f) * 4.0f);
        // Sprite2D's generated texture rectangles use unsigned coordinates.
        // Carry the signed horizontal position in the extended origin instead.
        gEXSetRectAlign((*dl)++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER,
            x0 - 640, 0, x0 - 640, 0);
        x1 -= x0;
        x0 = 0;
        width = 160;
        height = 240;
    }

    guSprite2DInit(&gDisplayListContext->sprites[gDisplayListContext->usedSprites],
        img, 0, width, width, height, 0, 2, 0, 0);
    gSPSprite2DBase((*dl)++, OS_K0_TO_PHYSICAL(&gDisplayListContext->sprites[gDisplayListContext->usedSprites]));
    gSPSprite2DScaleFlip((*dl)++, (f32)width / ((x1 - x0) * 0.25f) * 1024.0f,
        (f32)height / ((y1 - y0) * 0.25f) * 1024.0f, 0, 0);
    gSPSprite2DDraw((*dl)++, x0, y0);
    gDPPipeSync((*dl)++);
    ++gDisplayListContext->usedSprites;
    if (preview) {
        gEXSetRectAlign((*dl)++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER,
            -640, 0, -640, 0);
        gEXSetScissorAlign((*dl)++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER,
            -160, 0, -160, 0, 0, 0, 320, 240);
        gDPSetScissor((*dl)++, G_SC_NON_INTERLACE, 0, 0, 320, 240);
    }
}
