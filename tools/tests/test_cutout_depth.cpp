#include <cassert>
#include <cstdint>
struct Gfx { struct { uint32_t w0, w1; } words; };
#include "../../patches/sssv_cutout_depth.h"
int main() {
    Gfx fence[] = {{{0xB900031D,0xC81049D8}},{{0xB9000002,1}},{{0xBF000000,0}},{{0xB8000000,0}}};
    assert(sssv_fix_cutout_depth(fence,4)==1);
    assert(fence[0].words.w1==0xC81049F8);
    assert(sssv_fix_cutout_depth(fence,4)==0);
    Gfx mixed[] = {{{0xB900031D,0xC81049D8}},{{0xBF000000,0}},{{0xB9000002,1}},{{0xBF000000,0}},{{0xB8000000,0}}};
    assert(sssv_fix_cutout_depth(mixed,5)==0);
    Gfx glass[] = {{{0xB900031D,0xC81049D8}},{{0xB9000002,0}},{{0xB1000000,0}},{{0xB8000000,0}}};
    assert(sssv_fix_cutout_depth(glass,4)==0);
    glass[1].words.w1=3; // Dithered transparency is not a cutout.
    assert(sssv_fix_cutout_depth(glass,4)==0);
    Gfx decal[] = {{{0xB900031D,0xC8104DD8}},{{0xB9000002,1}},{{0xBF000000,0}},{{0xB8000000,0}}};
    assert(sssv_fix_cutout_depth(decal,4)==0);
    Gfx unterminated[] = {{{0xB900031D,0xC81049D8}},{{0xB9000002,1}},{{0xBF000000,0}}};
    assert(sssv_fix_cutout_depth(unterminated,3)==0);
}
