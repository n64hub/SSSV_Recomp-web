#include "sssv_patch_common.h"

#define GROUPED_QUEUE_CAPACITY 60
#define G_CCMUX_LOD_FRACTION 13u
#define G_ACMUX_LOD_FRACTION 0u

typedef struct { s32 x, z, y; } GroupedPosition;
typedef struct { s16 pitch, yaw; } GroupedRotation;
typedef struct {
    s8 tail;
    s8 last_index;
    s8 texture_heads[48];
    u8 texture[GROUPED_QUEUE_CAPACITY];
    s8 next[GROUPED_QUEUE_CAPACITY];
    u16 scale[GROUPED_QUEUE_CAPACITY];
    GroupedPosition position[GROUPED_QUEUE_CAPACITY];
    GroupedRotation rotation[GROUPED_QUEUE_CAPACITY];
    Gfx* model[GROUPED_QUEUE_CAPACITY];
} GroupedObjectQueue;

_Static_assert(offsetof(GroupedObjectQueue, texture) == 0x32, "Grouped texture offset");
_Static_assert(offsetof(GroupedObjectQueue, next) == 0x6E, "Grouped link offset");
_Static_assert(offsetof(GroupedObjectQueue, scale) == 0xAA, "Grouped scale offset");
_Static_assert(offsetof(GroupedObjectQueue, position) == 0x124, "Grouped position offset");
_Static_assert(offsetof(GroupedObjectQueue, rotation) == 0x3F4, "Grouped rotation offset");
_Static_assert(offsetof(GroupedObjectQueue, model) == 0x4E4, "Grouped model offset");
_Static_assert(sizeof(GroupedObjectQueue) == 0x5D4, "Grouped queue layout");

extern DisplayList* D_803D3434;
extern u8 D_800BA760[];
extern u64 gRenderMode1;
extern u64 gRenderMode2;

// Each native queue is bounded to sixty entries. Give each queue its own
// matrix bank in each buffered frame instead of truncating it when animal
// drawing has consumed the shared 250 matrices. Linker-reserved patch RAM is
// below 16 MiB physically, so these matrices need no extended address mode.
static Mtx s_grouped_object_matrices[2][2][GROUPED_QUEUE_CAPACITY]
    __attribute__((aligned(8)));
_Static_assert(sizeof(s_grouped_object_matrices) == 15360, "Grouped matrix capacity");

static void grouped_tile_sync(Gfx** dl) {
    (*dl)->words.w0 = 0xE8000000u;
    (*dl)->words.w1 = 0;
    (*dl)++;
}

RECOMP_EXPORT void sssv_render_grouped_object_queue(GroupedObjectQueue* queue, Gfx** dl) {
    s32 frame;
    s32 bank;
    s32 index = 0;
    s32 steps = 0;
    u8 loaded_texture = 0xFF;
    const u32 context = (u32)(uintptr_t)D_803D3434;
    const u32 address = (u32)(uintptr_t)queue;

    if (context == 0x80162B38u) frame = 0;
    else if (context == 0x8019E728u) frame = 1;
    else return;
    if (address == 0x803D3448u) bank = 0;
    else if (address == 0x803D3A20u) bank = 1;
    else return;
    if (queue->last_index < 0) return;

    // Preserve the native texture-group linked order, not array order. Bound
    // traversal as well as the matrix index in case a link is malformed.
    while (index >= 0 && index < GROUPED_QUEUE_CAPACITY && steps++ < GROUPED_QUEUE_CAPACITY) {
        const u8 texture = queue->texture[index] & 0x3F;
        Mtx* matrix = &s_grouped_object_matrices[frame][bank][index];
        const s32 scale = queue->scale[index] << 5;
        if (texture != loaded_texture) {
            loaded_texture = texture;
            gDPSetTextureImage((*dl)++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1,
                &D_800BA760[(u32)texture * 0xAB8u]);
            grouped_tile_sync(dl);
            gDPSetTile((*dl)++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0,
                G_TX_LOADTILE, 0, G_TX_WRAP, G_TX_NOMASK, G_TX_NOLOD,
                G_TX_WRAP, G_TX_NOMASK, G_TX_NOLOD);
            grouped_tile_sync(dl);
            gDPLoadBlock((*dl)++, G_TX_LOADTILE, 0, 0, 1372, 0);
            grouped_tile_sync(dl);
        }
        func_80125FE0(matrix,
            queue->position[index].x, queue->position[index].z, queue->position[index].y,
            queue->rotation[index].pitch, queue->rotation[index].yaw, scale, scale, scale);
        gSPMatrix((*dl)++, (void*)(uintptr_t)OS_K0_TO_PHYSICAL(matrix),
            G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
        gSPDisplayList((*dl)++, queue->model[index]);
        gSPPopMatrix((*dl)++, G_MTX_MODELVIEW);

        // Native TV screen material restoration deliberately targets gXluDL.
        if (queue->model[index] == (Gfx*)0x01017AA0u) {
            gDPSetCombineLERP(gXluDL++,
                TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0,
                TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0,
                COMBINED, 0, SHADE, 0, 0, 0, 0, SHADE);
            gDPSetRenderMode(gXluDL++, gRenderMode1, gRenderMode2);
        }
        index = queue->next[index];
    }
    // The native producer resets both grouped queues separately each frame.
}
