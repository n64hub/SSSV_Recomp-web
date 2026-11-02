#include "sssv_patch_common.h"
#include "sssv_render_context.h"
#include "sssv_rt64_tagging.h"

/* The original menu draws thirty wedges from eight mirrored bitmaps. The HD
 * path uses one shared annulus definition instead, so the outer and inner
 * boundaries stay circular even while individual biome arcs move outward. */
extern void func_8039C834_7ADEE4(Gfx** dl, u8* src, u16 width, u16 height,
    f32 scale_x, f32 scale_y, u8 flip_x, u8 flip_y, u16 x, u16 y);

typedef struct {
    s8 x;
    s8 y;
    u8 width;
    u8 height;
    u8 tile;
} MenuRingPosition;

static const MenuRingPosition s_positions[30] = {
    {-31,-7,9,7,7}, {-30,-13,10,9,6}, {-28,-19,10,11,5},
    {-26,-24,12,12,4}, {-21,-27,10,12,3}, {-16,-30,9,11,2},
    {-10,-31,7,11,1}, {-3,-31,7,10,0}, {3,-31,7,11,1},
    {7,-30,9,11,2}, {11,-27,10,12,3}, {14,-24,12,12,4},
    {18,-19,10,11,5}, {20,-13,10,9,6}, {22,-7,9,7,7},
    {22,-1,9,7,7}, {20,3,10,9,6}, {18,7,10,11,5},
    {14,11,12,12,4}, {11,14,10,12,3}, {7,18,9,11,2},
    {3,19,7,11,1}, {-3,20,7,10,0}, {-10,19,7,11,1},
    {-16,18,9,11,2}, {-21,14,10,12,3}, {-26,11,12,12,4},
    {-28,7,10,11,5}, {-30,3,10,9,6}, {-31,-1,9,7,7}
};

#define MENU_RING_CAPACITY 160
#define MENU_RING_VERTICES 30
#define MENU_RING_COMMANDS 64
#define MENU_RING_RADII 6
#define MENU_RING_ANGLES 5
#define MENU_RING_PRECISION 16.0f

/* A frame can contain four banks, each with thirty wedges. Three independent
 * pools retain geometry until all buffered graphics tasks have consumed it.
 * Secondary display lists avoid overflowing the native 4000-command mainDL. */
static Vtx s_ring_vertices[3][MENU_RING_CAPACITY][MENU_RING_VERTICES] __attribute__((aligned(8)));
static Gfx s_ring_commands[3][MENU_RING_CAPACITY][MENU_RING_COMMANDS] __attribute__((aligned(8)));
static Mtx s_ring_projection;
static Mtx s_ring_model;
static Mtx s_ring_identity;
static VpRaw s_ring_viewport __attribute__((aligned(8)));
static u32 s_ring_frame = 0xFFFFFFFFu;
static u32 s_ring_count;
static s32 s_ring_matrices_ready;
static s32 s_ring_pulse;

/* Clockwise from up, centre=(level-7)*12 degrees, with a 0.7-degree seam.
 * Four subdivisions per wedge bound the circle's chord error below 0.01px
 * at native scale. Literal unit vectors avoid importing host math routines. */
static const f32 s_ring_directions[30][MENU_RING_ANGLES][2] = {
    {{-0.999981342f, 0.006108614f}, {-0.998465026f, 0.055385848f}, {-0.994521895f, 0.104528463f}, {-0.988161535f, 0.153417018f}, {-0.979399403f, 0.201932685f}},
    {{-0.976859298f, 0.213882938f}, {-0.965130804f, 0.261768086f}, {-0.951056516f, 0.309016994f}, {-0.934670643f, 0.355514823f}, {-0.916013010f, 0.401148557f}},
    {{-0.911043816f, 0.412309551f}, {-0.889615735f, 0.456709802f}, {-0.866025404f, 0.500000000f}, {-0.840330159f, 0.542074925f}, {-0.812592453f, 0.582832313f}},
    {{-0.805411346f, 0.592716258f}, {-0.775220190f, 0.631691109f}, {-0.743144825f, 0.669130606f}, {-0.709263214f, 0.704943752f}, {-0.673657707f, 0.739043499f}},
    {{-0.664578537f, 0.747218421f}, {-0.626943802f, 0.779064483f}, {-0.587785252f, 0.809016994f}, {-0.547198064f, 0.837003153f}, {-0.505280886f, 0.862954938f}},
    {{-0.494700456f, 0.869063553f}, {-0.451266962f, 0.892389001f}, {-0.406736643f, 0.913545458f}, {-0.361217733f, 0.932481501f}, {-0.314820866f, 0.949151106f}},
    {{-0.303201591f, 0.952926437f}, {-0.255867590f, 0.966711837f}, {-0.207911691f, 0.978147601f}, {-0.159450453f, 0.987205932f}, {-0.110601664f, 0.993864816f}},
    {{-0.098451362f, 0.995141864f}, {-0.049285577f, 0.998784728f}, {0.000000000f, 1.000000000f}, {0.049285577f, 0.998784728f}, {0.098451362f, 0.995141864f}},
    {{0.110601664f, 0.993864816f}, {0.159450453f, 0.987205932f}, {0.207911691f, 0.978147601f}, {0.255867590f, 0.966711837f}, {0.303201591f, 0.952926437f}},
    {{0.314820866f, 0.949151106f}, {0.361217733f, 0.932481501f}, {0.406736643f, 0.913545458f}, {0.451266962f, 0.892389001f}, {0.494700456f, 0.869063553f}},
    {{0.505280886f, 0.862954938f}, {0.547198064f, 0.837003153f}, {0.587785252f, 0.809016994f}, {0.626943802f, 0.779064483f}, {0.664578537f, 0.747218421f}},
    {{0.673657707f, 0.739043499f}, {0.709263214f, 0.704943752f}, {0.743144825f, 0.669130606f}, {0.775220190f, 0.631691109f}, {0.805411346f, 0.592716258f}},
    {{0.812592453f, 0.582832313f}, {0.840330159f, 0.542074925f}, {0.866025404f, 0.500000000f}, {0.889615735f, 0.456709802f}, {0.911043816f, 0.412309551f}},
    {{0.916013010f, 0.401148557f}, {0.934670643f, 0.355514823f}, {0.951056516f, 0.309016994f}, {0.965130804f, 0.261768086f}, {0.976859298f, 0.213882938f}},
    {{0.979399403f, 0.201932685f}, {0.988161535f, 0.153417018f}, {0.994521895f, 0.104528463f}, {0.998465026f, 0.055385848f}, {0.999981342f, 0.006108614f}},
    {{0.999981342f, -0.006108614f}, {0.998465026f, -0.055385848f}, {0.994521895f, -0.104528463f}, {0.988161535f, -0.153417018f}, {0.979399403f, -0.201932685f}},
    {{0.976859298f, -0.213882938f}, {0.965130804f, -0.261768086f}, {0.951056516f, -0.309016994f}, {0.934670643f, -0.355514823f}, {0.916013010f, -0.401148557f}},
    {{0.911043816f, -0.412309551f}, {0.889615735f, -0.456709802f}, {0.866025404f, -0.500000000f}, {0.840330159f, -0.542074925f}, {0.812592453f, -0.582832313f}},
    {{0.805411346f, -0.592716258f}, {0.775220190f, -0.631691109f}, {0.743144825f, -0.669130606f}, {0.709263214f, -0.704943752f}, {0.673657707f, -0.739043499f}},
    {{0.664578537f, -0.747218421f}, {0.626943802f, -0.779064483f}, {0.587785252f, -0.809016994f}, {0.547198064f, -0.837003153f}, {0.505280886f, -0.862954938f}},
    {{0.494700456f, -0.869063553f}, {0.451266962f, -0.892389001f}, {0.406736643f, -0.913545458f}, {0.361217733f, -0.932481501f}, {0.314820866f, -0.949151106f}},
    {{0.303201591f, -0.952926437f}, {0.255867590f, -0.966711837f}, {0.207911691f, -0.978147601f}, {0.159450453f, -0.987205932f}, {0.110601664f, -0.993864816f}},
    {{0.098451362f, -0.995141864f}, {0.049285577f, -0.998784728f}, {0.000000000f, -1.000000000f}, {-0.049285577f, -0.998784728f}, {-0.098451362f, -0.995141864f}},
    {{-0.110601664f, -0.993864816f}, {-0.159450453f, -0.987205932f}, {-0.207911691f, -0.978147601f}, {-0.255867590f, -0.966711837f}, {-0.303201591f, -0.952926437f}},
    {{-0.314820866f, -0.949151106f}, {-0.361217733f, -0.932481501f}, {-0.406736643f, -0.913545458f}, {-0.451266962f, -0.892389001f}, {-0.494700456f, -0.869063553f}},
    {{-0.505280886f, -0.862954938f}, {-0.547198064f, -0.837003153f}, {-0.587785252f, -0.809016994f}, {-0.626943802f, -0.779064483f}, {-0.664578537f, -0.747218421f}},
    {{-0.673657707f, -0.739043499f}, {-0.709263214f, -0.704943752f}, {-0.743144825f, -0.669130606f}, {-0.775220190f, -0.631691109f}, {-0.805411346f, -0.592716258f}},
    {{-0.812592453f, -0.582832313f}, {-0.840330159f, -0.542074925f}, {-0.866025404f, -0.500000000f}, {-0.889615735f, -0.456709802f}, {-0.911043816f, -0.412309551f}},
    {{-0.916013010f, -0.401148557f}, {-0.934670643f, -0.355514823f}, {-0.951056516f, -0.309016994f}, {-0.965130804f, -0.261768086f}, {-0.976859298f, -0.213882938f}},
    {{-0.979399403f, -0.201932685f}, {-0.988161535f, -0.153417018f}, {-0.994521895f, -0.104528463f}, {-0.998465026f, -0.055385848f}, {-0.999981342f, -0.006108614f}},
};
static const f32 s_ring_radii[MENU_RING_RADII] = {21.5f, 21.9f, 22.8f, 29.3f, 30.5f, 31.0f};
static const f32 s_ring_shades[MENU_RING_RADII] = {28.0f, 115.0f, 240.0f, 205.0f, 245.0f, 36.0f};

static void menu_ring_pack_matrix(Mtx* target, const f32* source) {
    u32* words = (u32*)target;
    s32 i;
    for (i = 0; i < 8; ++i) {
        const s32 a = (s32)(source[i * 2] * 65536.0f);
        const s32 b = (s32)(source[i * 2 + 1] * 65536.0f);
        words[i] = ((u32)a & 0xFFFF0000u) | (((u32)b >> 16) & 0xFFFFu);
        words[i + 8] = ((u32)a << 16) | ((u32)b & 0xFFFFu);
    }
}

static s16 menu_ring_coordinate(f32 value) {
    value *= MENU_RING_PRECISION;
    return (s16)(value >= 0.0f ? value + 0.5f : value - 0.5f);
}

static void menu_ring_mesh_begin(void) {
    if (!s_ring_matrices_ready) {
        const f32 projection[16] = {
            1.0f / 160.0f, 0, 0, 0,
            0, -1.0f / 120.0f, 0, 0,
            0, 0, -1, 0, -1, 1, 0, 1
        };
        const f32 identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
        const f32 model[16] = {
            1.0f / MENU_RING_PRECISION,0,0,0,
            0,1.0f / MENU_RING_PRECISION,0,0,
            0,0,1,0, 0,0,0,1
        };
        menu_ring_pack_matrix(&s_ring_projection, projection);
        menu_ring_pack_matrix(&s_ring_model, model);
        menu_ring_pack_matrix(&s_ring_identity, identity);
        s_ring_viewport.vscale[0] = s_ring_viewport.vtrans[0] = 640;
        s_ring_viewport.vscale[1] = s_ring_viewport.vtrans[1] = 480;
        s_ring_viewport.vscale[2] = s_ring_viewport.vtrans[2] = 511;
        s_ring_viewport.vscale[3] = s_ring_viewport.vtrans[3] = 0;
        s_ring_matrices_ready = 1;
    }
    rc_ensure_rt64_extended_enabled(&gMainDL);
    gDPPipeSync(gMainDL++);
    gEXPushProjectionMatrix(gMainDL++);
    gEXSetRDRAMExtended(gMainDL++, 1);
    gEXSetViewportAlign(gMainDL++, G_EX_ORIGIN_CENTER, -640, 0);
    gSPViewport(gMainDL++, &s_ring_viewport);
    rt64_tag_projection_matrix(&gMainDL, 0x00001015u);
    gSPMatrix(gMainDL++, &s_ring_projection, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    /* Sprite2D reuses POPMTX as its draw opcode, so do not push/pop the RSP
     * model stack here. Its sprites never consume modelview matrices. */
    gSPMatrix(gMainDL++, &s_ring_model, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gImmp1_(gMainDL++, 0xB6, 0x000F3001u);
    /* Sprite2D can leave vertex shading disabled. The mesh's combiner needs
     * both vertex RGBA and smooth interpolation across the bevel bands. */
    gImmp1_(gMainDL++, 0xB7, 0x00000204u);
    gSPTexture(gMainDL++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetCycleType(gMainDL++, G_CYC_1CYCLE);
    gDPSetAlphaCompare(gMainDL++, G_AC_NONE);
    gDPSetRenderMode(gMainDL++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);
    /* Vertex grayscale is multiplied by the native completion/selection tint. */
    gDPSetCombineLERP(gMainDL++, SHADE, 0, PRIMITIVE, 0, SHADE, 0, PRIMITIVE, 0,
        SHADE, 0, PRIMITIVE, 0, SHADE, 0, PRIMITIVE, 0);
}

static void menu_ring_mesh_end(void) {
    gDPPipeSync(gMainDL++);
    gSPMatrix(gMainDL++, &s_ring_identity, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gEXPopProjectionMatrix(gMainDL++);
    gEXSetRDRAMExtended(gMainDL++, 0);
    gEXSetViewportAlign(gMainDL++, G_EX_ORIGIN_NONE, 0, 0);
    gSPViewport(gMainDL++, &gMainViewport);
    gSPTexture(gMainDL++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPSetTexturePersp(gMainDL++, G_TP_NONE);
    gDPSetAlphaCompare(gMainDL++, G_AC_THRESHOLD);
    /* Expanded native RM_TEX_EDGE(1/2), absent from the patch mini-header. */
    gDPSetRenderMode(gMainDL++, 0x0C087008u, 0x03027008u);
    gDPSetCombineLERP(gMainDL++, TEXEL0, 0, PRIMITIVE, 0, TEXEL0, 0, PRIMITIVE, 0,
        TEXEL0, 0, PRIMITIVE, 0, TEXEL0, 0, PRIMITIVE, 0);
}

static void menu_ring_draw_mesh(s32 level, s16 x, s16 y, f32 scale) {
    const u32 frame = rc_frame_index() % 3;
    const u32 slot = s_ring_count++;
    Vtx* vertices = s_ring_vertices[frame][slot];
    Gfx* commands = s_ring_commands[frame][slot];
    Gfx* dl = commands;
    s32 a, r;
    for (a = 0; a < MENU_RING_ANGLES; ++a) {
        const f32 dx = s_ring_directions[level][a][0];
        const f32 up = s_ring_directions[level][a][1];
        const f32 light = 0.75f + 0.20f * (-0.6f * dx + 0.8f * up);
        const f32 edge = (a == 0 || a == MENU_RING_ANGLES - 1) ? 0.58f : 1.0f;
        for (r = 0; r < MENU_RING_RADII; ++r) {
            Vtx* vertex = &vertices[a * MENU_RING_RADII + r];
            const f32 radius = s_ring_radii[r] * scale;
            const u8 shade = (u8)(s_ring_shades[r] * light * edge + 0.5f);
            vertex->v.ob[0] = menu_ring_coordinate(x + dx * radius);
            vertex->v.ob[1] = menu_ring_coordinate(y - 0.5f * scale - up * radius);
            vertex->v.ob[2] = 0;
            vertex->v.flag = 0;
            vertex->v.tc[0] = vertex->v.tc[1] = 0;
            vertex->v.cn[0] = vertex->v.cn[1] = vertex->v.cn[2] = shade;
            vertex->v.cn[3] = 255;
        }
    }
    rt64_tag_model_matrix(&dl, 0x10020000u + slot, 0);
    gSPMatrix(dl++, &s_ring_model, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    for (a = 0; a < MENU_RING_ANGLES - 1; ++a) {
        /* The active menu microcode is legacy Sprite2D/F3D, not F3DEX.
         * Load two adjacent radial columns into its 16-entry vertex cache.
         * F3D encodes n-1 in bits20..23 and triangle indices multiplied10. */
        dl->words.w0 = 0x04B000C0u; /* VTX: 12 vertices, destination0. */
        dl->words.w1 = (u32)(uintptr_t)&vertices[a * MENU_RING_RADII];
        ++dl;
        for (r = 0; r < MENU_RING_RADII - 1; ++r) {
            dl->words.w0 = 0xBF000000u;
            dl->words.w1 = ((u32)(r * 10) << 16) | ((u32)((r + 1) * 10) << 8) | ((r + MENU_RING_RADII) * 10);
            ++dl;
            dl->words.w0 = 0xBF000000u;
            dl->words.w1 = ((u32)((r + MENU_RING_RADII) * 10) << 16) | ((u32)((r + 1) * 10) << 8) | ((r + MENU_RING_RADII + 1) * 10);
            ++dl;
        }
    }
    rt64_pop_model_matrix(&dl);
    gImmp1_(dl++, 0xB8, 0);
    gSPDisplayList(gMainDL++, commands);
}

RECOMP_PATCH void draw_level_rings_segment(u8 red, u8 green, u8 blue, u8 alpha,
    s16 start, s16 end, u8* eeprom, s16 xRot, s16 yRot, f32 scale, u8 currentLevel) {
    const u8* available = (const u8*)0x803F7DE0;
    u8* center_image = (u8*)0x800BA760;
    s32 i;
    s32 mesh = 0;
    const u32 frame = rc_frame_index();
    (void)alpha; /* The native routine does not use this argument. */
    s_ring_pulse = (s_ring_pulse + 1) % 100;
    if (frame != s_ring_frame) {
        s_ring_frame = frame;
        s_ring_count = 0;
    }
    if (start != 30 && start >= 0 && end < 30 && start <= end) {
        mesh = recomp_get_hd_title_enabled() && s_ring_count + end - start + 1 <= MENU_RING_CAPACITY;
    }
    if (mesh) menu_ring_mesh_begin();
    for (i = start; i <= end; ++i) {
        const s32 completed = (eeprom[4 + i] & 2) != 0;
        if (i == currentLevel) {
            if (s_ring_pulse < 50) {
                gDPSetPrimColor(gMainDL++, 0, 0, red, green, blue, 240);
            } else {
                gDPSetPrimColor(gMainDL++, 0, 0, 255, 128, 0, 240);
            }
        } else if (available[i] == 0 && currentLevel != 0xFF) {
            gDPSetPrimColor(gMainDL++, 0, 0, red, green, blue, 20);
        } else if (currentLevel == 0xFF) {
            if (completed) {
                if (i == 30) {
                    gDPSetPrimColor(gMainDL++, 0, 0, red - 15, 40, 40, 255);
                } else {
                    gDPSetPrimColor(gMainDL++, 0, 0, red + 15, 0, 0, 255);
                }
            } else {
                gDPSetPrimColor(gMainDL++, 0, 0, red, green, blue, 255);
            }
        } else {
            gDPSetPrimColor(gMainDL++, 0, 0, red, green, blue, completed ? 240 : 70);
        }

        if (start == 30) {
            func_8039C834_7ADEE4(&gMainDL, center_image, 43, 43, scale, scale, 0, 0, xRot, yRot);
        } else if (mesh) {
            menu_ring_draw_mesh(i, xRot, yRot, scale);
        } else {
            const MenuRingPosition* position = &s_positions[i];
            func_8039C834_7ADEE4(&gMainDL, center_image + 0xFA0 + position->tile * 0x4B0,
                position->width * 2, position->height * 2, scale / 2, scale / 2,
                position->x < 0, position->y < -1,
                (s16)(s32)(xRot + position->x * scale), (s16)(s32)(yRot + position->y * scale));
        }
    }
    if (mesh) menu_ring_mesh_end();
}

