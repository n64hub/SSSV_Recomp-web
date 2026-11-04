#include "sssv_patch_common.h"
#include "sssv_cutout_depth.h"

RECOMP_EXPORT void sssv_fix_world_cutout_depth(void) {
    for (int x = 0; x < 4; ++x) {
        for (int z = 0; z < 6; ++z) {
            Gfx* list = gWorldCellTranslucentDisplayLists[x][z];
            u32 address = (u32)(uintptr_t)list;
            if (address < 0x80000000u || address >= 0x80800000u || (address & 7u)) continue;
            int capacity = MIN(4096, (0x80800000u - address) / sizeof(Gfx));
            sssv_fix_cutout_depth(list, capacity);
        }
    }
}
