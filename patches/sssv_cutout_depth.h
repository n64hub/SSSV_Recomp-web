#ifndef SSSV_CUTOUT_DEPTH_H
#define SSSV_CUTOUT_DEPTH_H

/* Generated cell lists are flat. Only enable writes when every triangle
 * governed by this render-mode command uses threshold alpha rejection.
 * Mixed translucent runs, dithered surfaces and decal modes stay untouched. */
static inline int sssv_fix_cutout_depth(Gfx* list, int capacity) {
    Gfx* mode = 0;
    int threshold = 0, cutout = 0, translucent = 0, changed = 0;
    for (int i = 0; i < capacity; ++i) {
        Gfx* cmd = list + i;
        unsigned op = cmd->words.w0 >> 24;
        if (cmd->words.w0 == 0xB900031Du || op == 0xB8u) {
            if (mode && cutout && !translucent) {
                mode->words.w1 |= 0x20u; /* Z_UPD */
                ++changed;
            }
            mode = 0; cutout = 0; translucent = 0;
            if (op == 0xB8u) return changed;
            if ((cmd->words.w1 & 0x00FFFFFFu) == 0x001049D8u) mode = cmd;
        } else if (cmd->words.w0 == 0xB9000002u) {
            threshold = (cmd->words.w1 & 3u) == 1u;
        } else if (op == 0xBFu || op == 0xB1u) {
            if (threshold) cutout = 1;
            else translucent = 1;
        } else if (op == 0x06u) {
            /* Do not infer the material state of nested lists. */
            mode = 0;
        }
    }
    return changed;
}
#endif
