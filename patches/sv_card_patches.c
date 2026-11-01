#include "sssv_patch_common.h"
#include "sssv_render_context.h"

/* Keep the animated TV geometry, but give its two halves one continuous
 * coordinate system when they sample the single HD card. Never modify the
 * original vertices: switching the pack off must use the original model. */
#define SV_CARD_SLOTS 96
static Vtx s_card_vertices[SV_CARD_SLOTS][8];
static Gfx s_card_lists[SV_CARD_SLOTS][28];
static u32 s_card_cursor;

RECOMP_PATCH void render_tv_glass(Gfx** dl, s16 x, s16 y, s16 z) {
    Gfx* card = (Gfx*)0x0400A9E0;
    if (recomp_get_hd_title_enabled()) {
        const u32 slot = s_card_cursor++ % SV_CARD_SLOTS;
        Vtx* vertices = s_card_vertices[slot];
        Vtx* original = (Vtx*)((u8*)gMenuSegmentBase + 0xA960);
        s32 i;
        for (i = 0; i < 8; ++i) {
            vertices[i] = original[i];
            /* gsSPTexture uses a half-scale, so one texel is 64 units. */
            vertices[i].v.tc[0] = (s16)(((28 - vertices[i].v.ob[0]) * 4096 + 29) / 58);
            vertices[i].v.tc[1] = (s16)(((22 - vertices[i].v.ob[2]) * 4096 + 25) / 50 - (i >= 4 ? 2048 : 0));
        }
        card = s_card_lists[slot];
        for (i = 0; i < 28; ++i) card[i] = ((Gfx*)((u8*)gMenuSegmentBase + 0xA9E0))[i];
        card[12].words.w1 = OS_K0_TO_PHYSICAL(vertices);
        card[22].words.w1 = OS_K0_TO_PHYSICAL(vertices + 4);
    }
    func_80125FE0(&gDisplayListContext->modelViewMtx[gDisplayListContext->usedModelViewMtxs],
        x << 16, y << 16, z << 16, 0, 0, 32768, 32768, 36864);
    gSPMatrix((*dl)++, &gDisplayListContext->modelViewMtx[gDisplayListContext->usedModelViewMtxs++],
        G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
    gSPDisplayList((*dl)++, card);
    gSPPopMatrix((*dl)++, G_MTX_MODELVIEW);
    gDPPipeSync((*dl)++);
}
