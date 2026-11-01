#include "sssv_patch_common.h"

// currentFont.glyphBitmapBase. Keep the native font metrics and text renderer;
// only this caption uses a separate texture identity when HD Textures is on.
// select_comic_sans_font writes the verified US 1.0 field at this address.
#define HEADER_FONT_BITMAP (*(u8**)0x8023F1E4)
#define HEADER_LAST_TILE (0x110 + 'Y')

static u8 s_header_glyphs[HEADER_LAST_TILE + 1][128] __attribute__((aligned(8)));
static u8 s_header_glyphs_ready;

u8* sssv_begin_hd_header_font(void) {
    static const char caption[] = "SPACE STATION SILICON VALLEY";
    u8* original;
    s32 i, j;

    if (!recomp_get_hd_title_enabled()) {
        return 0;
    }

    original = HEADER_FONT_BITMAP;
    if (!s_header_glyphs_ready) {
        for (i = 0; caption[i] != 0; ++i) {
            const s32 tile = 0x110 + caption[i];
            for (j = 0; j < 128; ++j) {
                s_header_glyphs[tile][j] = original[tile * 128 + j];
            }
            // A single otherwise unused I4 bit gives this immutable copy a
            // distinct replacement hash. Spaces stay completely transparent.
            if (caption[i] != ' ') {
                s_header_glyphs[tile][127] ^= 1;
            }
        }
        s_header_glyphs_ready = 1;
    }
    HEADER_FONT_BITMAP = &s_header_glyphs[0][0];
    return original;
}

void sssv_end_hd_header_font(u8* original) {
    if (original != 0) {
        HEADER_FONT_BITMAP = original;
    }
}
