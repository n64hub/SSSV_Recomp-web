#ifndef __SSSV_HOOKS_H__
#define __SSSV_HOOKS_H__

// What sssv.us.toml's one-line [[patches.hook]] texts call in the recompiled C (the multi-line ones declare their own):
// the game's C++ in src/game and librecomp's overlay loader. With Emscripten the recompiled code is built with
// -Werror=implicit-function-declaration and this header included first (CMakeLists.txt), since wasm-ld turns a call
// through an undeclared function into a trap at run time. A new hook's function is declared here too.

#include "recomp.h"

void load_overlays(uint32_t rom, int32_t ram_addr, uint32_t size);
void sssv_begin_main_display_list(uint8_t* rdram, recomp_context* ctx);
void sssv_submit_main_display_list(uint8_t* rdram, recomp_context* ctx);
void sssv_file_select_layout_hook(uint8_t* rdram, recomp_context* ctx, int stage);
void sssv_hd_header_hook(uint8_t* rdram, recomp_context* ctx);
void sssv_hd_menu_title_hook(uint8_t* rdram, recomp_context* ctx);
void sssv_health_layout_hook(uint8_t* rdram, recomp_context* ctx, int end);
void sssv_intro_skip_dog_hook(uint8_t* rdram, recomp_context* ctx);
void sssv_intro_skip_hook(uint8_t* rdram, recomp_context* ctx);
void sssv_layout_demo_prompt(uint8_t* rdram, recomp_context* ctx);
void sssv_patch_trophy_collision_guard(uint8_t* rdram, recomp_context* ctx);
void sssv_reflection_animal_range(uint8_t* rdram, recomp_context* ctx, int phase);

#endif
