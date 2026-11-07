// Exercise the real skip state machine with the game's reload/audio calls stubbed.
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#define __SSSV_PATCH_COMMON_H__
#define RECOMP_EXPORT

typedef uint8_t u8;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
struct { int level; } gGameState;
struct { int unk0; } gOverlayMenuState;
struct { int transitionId, unk0, unk2, unk6; } gScreenTransition;
struct Pad { unsigned button; } pad;
struct Pad* gControllerInput = &pad;
void* gMainDL;
void draw_rectangle(void*, int, int, int, int, int, int, int, int);
void start_sequence_volume_fade(int, float, float, float);
void start_sfx_volume_fade(float, float, float);

#include "../../patches/intro_skip_patches.c"

IntroPlayerSave D_8023F260;
s16 gInputMode;
u8 gInitialisationState, D_802912DE, D_803D5538;
s32 gLevelProgress = 20;
static int music_track, music_scale, pending_sequence, stop_calls, reloads;

void draw_rectangle(void* p, int a,int b,int c,int d,int e,int f,int g,int h) {}
void start_sequence_volume_fade(int a,float b,float c,float d) {}
void start_sfx_volume_fade(float a,float b,float c) {}
void stop_all_sounds(void) { pending_sequence = 0; ++stop_calls; }
void set_music_track(s8 track) { music_track = track; }
void set_game_state(void* entity, s16 state, s32 value) {
    if (state == 0x7FA2) D_8023F260.script_progress = value;
    else if (state == 0x7FA1) music_scale = value;
    else { assert(state == 0x7FA3); gLevelProgress = value; }
}
void load_smashing_start(void) {
    ++reloads;
    // The regression: reload can undo the cleanup performed before it.
    music_track = 1; music_scale = 2048; pending_sequence = 1;
}

int main(void) {
    gGameState.level = 1;
    D_8023F260.script_progress = 0x20;
    sssv_update_intro_skip(); // Release arms skip.
    pad.button = 0x1000; D_802912DE = 1;
    sssv_update_intro_skip(); // Press starts the fade.
    assert(reloads == 0 && gScreenTransition.transitionId == 3);
    gScreenTransition.unk0 = 1; // Fade completes.
    sssv_update_intro_skip();
    assert(reloads == 1 && stop_calls == 2);
    assert(music_track == 0 && music_scale == 0 && pending_sequence == 0);
    assert(D_8023F260.script_progress == 0x21);
    assert(!(pad.button & 0x1000));
    assert(gLevelProgress == 20); // No briefing before the player is visible.
    D_803D5538 = 1;
    sssv_intro_skip_dog_drawn();
    sssv_update_intro_skip();
    for (int frame = 0; frame < 10; ++frame) {
        sssv_draw_intro_skip_blackout();
        sssv_update_intro_skip();
    }
    assert(gLevelProgress == (20 | 64));
    gLevelProgress &= ~64; // The original briefing handler consumes the request.
    sssv_update_intro_skip();
    assert(gLevelProgress == 20); // It is requested exactly once.
    load_smashing_start(); // Direct level entry does not arm a new briefing.
    sssv_update_intro_skip();
    assert(gLevelProgress == 20);
    // The normal ship script must still be able to start/fade music afterward.
    set_music_track(25); set_game_state(NULL, 0x7FA1, 32);
    assert(music_track == 25 && music_scale == 32);
    return 0;
}
