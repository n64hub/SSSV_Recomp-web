#include "sssv_patch_common.h"
#include "sssv_rt64_tagging.h"

/* Decorative repeats only: the nine scripted objects and their destruction
 * remain native. Render just the portion of the infinite grid in the frustum. */
typedef struct {
    s16 distance; u16 scale;
    s32 x, z, y;
    s16 pitch, yaw;
    Gfx* model;
    u8 texture; u8 pad[3];
} IntroGlassInstance;
_Static_assert(sizeof(IntroGlassInstance) == 28, "Native sorted instance layout");
extern f32 sqrtf(f32);
extern f32 __sinf(f32);
extern f32 __cosf(f32);
extern u32 gGeometryMode;
#define ABS(v) ((v)<0?-(v):(v))
#define ABSF(v) ABS(v)
#define G_CCMUX_LOD_FRACTION 13u
#define G_RM_ZB_XLU_SURF2 0x00104A50u
#define G_CULL_BACK 0x00002000u
static void glass_geometry(Gfx* p, u32 bits, s32 set) { p->words.w0=set?0xB7000000u:0xB6000000u; p->words.w1=bits; }
static void glass_tile_sync(Gfx* p) { p->words.w0=0xE8000000u; p->words.w1=0; }
#define GLASS_TILE_CAPACITY 1024
static Mtx s_glass_matrices[2][GLASS_TILE_CAPACITY] __attribute__((aligned(8)));
static s32 glass_floor(f32 v) { s32 n = (s32)v; return n - (v < n); }

RECOMP_EXPORT void sssv_extend_intro_glass(void) {
    IntroGlassInstance* queue = (IntroGlassInstance*)0x803D5188;
    s16 count = *(s16*)0x803D5508;
    s32 frame = (u32)(uintptr_t)gDisplayListContext == 0x80162B38u ? 0 : 1;
    s32 plane, cursor = 0;
    f32 aspect = recomp_get_target_aspect_ratio(4.0f / 3.0f);
    if (gGameState.level != 1 || aspect <= 4.0f / 3.0f || count <= 0 || count > 32) return;
    for (plane = 0; plane < 2; ++plane) {
        s32 i, found = -1;
        f32 cx = plane ? 3040.0f : 2999.0f;
        f32 cz = plane ? 1376.0f : 4222.0f;
        f32 height = plane ? 2564.0f : 1686.0f;
        for (i = 0; i < count; ++i) {
            if (queue[i].model == (Gfx*)0x05009720u && queue[i].scale == 16384 &&
                queue[i].pitch == 0 && queue[i].yaw == 0 &&
                ABS((queue[i].y >> 16) - (s32)height) < 4) { found = i; break; }
        }
        if (found < 0) continue;
        IntroGlassInstance* source = &queue[found];
        f32 fx = *(f32*)0x80204218, fz = *(f32*)0x8020421C, fy = *(f32*)0x80204220;
        f32 ux = *(f32*)0x80204200, uz = *(f32*)0x80204204, uy = *(f32*)0x80204208;
        f32 rx = fz * uy - fy * uz, rz = fy * ux - fx * uy, ry = fx * uz - fz * ux;
        f32 length = sqrtf(rx*rx + rz*rz + ry*ry);
        if (length < 0.001f) continue;
        rx /= length; rz /= length; ry /= length;
        /* Orthogonalize screen-up, including the cutscene's camera roll. */
        ux = rz*fy - ry*fz; uz = ry*fx - rx*fy; uy = rx*fz - rz*fx;
        f32 angle = gLevelConfig.fovY * 0.00872664626f;
        f32 tangent = __sinf(angle) / __cosf(angle);
        f32 minx = cx - 512, maxx = cx + 512, minz = cz - 512, maxz = cz + 512;
        for (i = 0; i < 4; ++i) {
            f32 sx = (i & 1) ? 1.0f : -1.0f, sy = (i & 2) ? 1.0f : -1.0f;
            f32 dx = fx + tangent * (rx * sx * aspect + ux * sy);
            f32 dz = fz + tangent * (rz * sx * aspect + uz * sy);
            f32 dy = fy + tangent * (ry * sx * aspect + uy * sy);
            f32 t = ABSF(dy) > 0.0001f ? (height - gCameraEyeWorldY) / dy : -1.0f;
            if (t <= 0 || t > 24000) t = 24000;
            f32 x = gCameraEyeWorldX + t * dx, z = gCameraEyeWorldZ + t * dz;
            minx = MIN(minx, x); maxx = MAX(maxx, x); minz = MIN(minz, z); maxz = MAX(maxz, z);
        }
        s32 left = MAX(-58, glass_floor((minx-cx)/512)-1), right = MIN(58, glass_floor((maxx-cx)/512)+1);
        s32 top = MAX(-58, glass_floor((minz-cz)/512)-1), bottom = MIN(58, glass_floor((maxz-cz)/512)+1);
        gSPDisplayList(gLayer0DL++, (Gfx*)0x010049A0);
        glass_geometry(gLayer0DL++, gGeometryMode,1);
        gSPDisplayList(gLayer0DL++, (Gfx*)0x01004510);
        gDPSetRenderMode(gLayer0DL++, G_RM_PASS, G_RM_ZB_XLU_SURF2);
        gDPSetCombineLERP(gLayer0DL++, SHADE,0,COMBINED,0, 0,0,0,PRIMITIVE,
            TEXEL1,TEXEL0,LOD_FRACTION,TEXEL0, 0,0,0,PRIMITIVE);
        gDPSetPrimColor(gLayer0DL++,0,0,0,0,0,128);
        gDPSetTextureImage(gLayer0DL++,G_IM_FMT_RGBA,G_IM_SIZ_16b,1,
            (u8*)0x800BA760 + (source->texture & 63)*0xAB8);
        glass_tile_sync(gLayer0DL++);
        gDPSetTile(gLayer0DL++,G_IM_FMT_RGBA,G_IM_SIZ_16b,0,0,G_TX_LOADTILE,0,
            G_TX_WRAP,G_TX_NOMASK,G_TX_NOLOD,G_TX_WRAP,G_TX_NOMASK,G_TX_NOLOD);
        gDPLoadSync(gLayer0DL++); gDPLoadBlock(gLayer0DL++,G_TX_LOADTILE,0,0,1372,0);
        gDPPipeSync(gLayer0DL++); glass_geometry(gLayer0DL++,G_CULL_BACK,0);
        s32 x, z;
        for (z=top; z<=bottom; ++z) for (x=left; x<=right; ++x) {
            if (x>=-1 && x<=1 && z>=-1 && z<=1) continue;
            f32 wx=cx+x*512, wz=cz+z*512;
            f32 dx=wx-gCameraEyeWorldX, dz=wz-gCameraEyeWorldZ, dy=height-gCameraEyeWorldY;
            f32 depth=dx*fx+dz*fz+dy*fy;
            if (depth < -600 || ABSF(dx*rx+dz*rz+dy*ry) > MAX(depth,0)*tangent*aspect+800 ||
                ABSF(dx*ux+dz*uz+dy*uy) > MAX(depth,0)*tangent+800) continue;
            if (cursor >= GLASS_TILE_CAPACITY || wx < -32000 || wx > 32000 || wz < -32000 || wz > 32000) continue;
            Mtx* matrix=&s_glass_matrices[frame][cursor++];
            func_80125FE0(matrix,(s32)wx * 65536,(s32)wz * 65536,source->y,0,0,source->scale<<5,source->scale<<5,source->scale<<5);
            rt64_tag_model_matrix(&gLayer0DL,0x04000000u+plane*65536u+(x+64)*128u+(z+64),0);
            gSPMatrix(gLayer0DL++,matrix,G_MTX_PUSH|G_MTX_MUL|G_MTX_MODELVIEW);
            gSPDisplayList(gLayer0DL++,source->model);
            gSPPopMatrix(gLayer0DL++,G_MTX_MODELVIEW); rt64_pop_model_matrix(&gLayer0DL);
        }
        glass_geometry(gLayer0DL++,G_CULL_BACK,1);
    }
}
