#include "sssv_patch_common.h"
#include "sssv_rt64_tagging.h"

// Native station-glass reflections replay the completed animal lists with a
// reflected projection. Keep that projection separate from the main camera
// for interpolation, and explicitly restore state instead of the native
// unmatched MODELVIEW pop (our animal lists balance their own stacks).
static void window_projection(DisplayList* frame, Mtx* reflection, u32 id) {
    gSPMatrix(gMainDL++, &frame->unk37410, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
    gSPMatrix(gMainDL++, &frame->unk37450, G_MTX_NOPUSH | G_MTX_MUL | G_MTX_PROJECTION);
    gSPMatrix(gMainDL++, &frame->unk37490, G_MTX_NOPUSH | G_MTX_MUL | G_MTX_PROJECTION);
    if (reflection) gSPMatrix(gMainDL++, reflection, G_MTX_NOPUSH | G_MTX_MUL | G_MTX_PROJECTION);
    gSPMatrix(gMainDL++, &frame->unk374D0, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    rt64_tag_projection_matrix(&gMainDL, id);
}

static void window_light(void* light, u32 slot) {
    gMainDL->words.w0 = 0x03800010u | ((0x84u + slot * 2u) << 16);
    gMainDL->words.w1 = OS_K0_TO_PHYSICAL(light);
    gMainDL++;
}

static void window_one_light(void) {
    gMainDL->words.w0 = 0xBC000002u;
    gMainDL->words.w1 = 0x80000040u; // F3DEX NUML(1).
    gMainDL++;
}

RECOMP_PATCH void render_ship_window_projection_replay(void) {
    const u16 mask = gCameraVisibilityMask[6];
    if (!(mask & 3)) return;
    DisplayList* frame = gDisplayListContext;
    u8* base = (u8*)frame;
    Mtx* reflection = (Mtx*)(base + 0x37510);
    u32* words = (u32*)reflection;
    s16* integer = (s16*)reflection;
    s32 i;
    for (i = 0; i < 16; ++i) words[i] = 0;
    integer[0] = (mask & 1) ? 1 : -1;
    integer[5] = 1;
    integer[10] = (mask & 1) ? -1 : 1;
    integer[15] = 1;
    integer[(mask & 1) ? 14 : 12] = (mask & 0xFFCu) << 2;

    if (!(mask & 1)) {
        // Match native dimmed reflection lighting, stored per buffered frame.
        u8* ambient = base + 0x3B6B0;
        u8* light = base + 0x3B6B8;
        for (i = 0; i < 8; ++i) ambient[i] = base[0x3B638 + i];
        for (i = 0; i < 16; ++i) light[i] = base[0x3B640 + i];
        for (i = 0; i < 3; ++i) {
            ambient[i] >>= 1; ambient[i + 4] >>= 1;
            light[i] >>= 2; light[i + 4] >>= 2;
        }
        window_one_light();
        window_light(light, 1);
        window_light(ambient, 2);
    }
    window_projection(frame, reflection, (mask & 1) ?
        SSSV_PROJECTION_REFLECTION_Z_ID : SSSV_PROJECTION_REFLECTION_X_ID);
    if (mask & 1) {
        // Skip only the three canonical camera matrices added by the XLU
        // state barrier; otherwise replaying the list would undo reflection.
        Gfx* xlu = (Gfx*)(base + 0xBB80);
        if (xlu[0].words.w0 == 0x01030040u &&
            xlu[1].words.w0 == 0x01010040u && xlu[2].words.w0 == 0x01010040u) xlu += 3;
        gSPDisplayList(gMainDL++, xlu);
    }
    gSPDisplayList(gMainDL++, (Gfx*)(base + 0x9600));
    if (!(mask & 1)) {
        window_one_light();
        window_light(base + 0x3B640, 1);
        window_light(base + 0x3B638, 2);
    }
    window_projection(frame, NULL, SSSV_PROJECTION_WORLD_TRANSFORM_ID);
}

