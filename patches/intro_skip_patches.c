#include "sssv_patch_common.h"

typedef struct {
    u8 prefix[0x34];
    u32 script_progress;
    u8 suffix[0x08];
} IntroPlayerSave;

extern IntroPlayerSave D_8023F260;
extern s16 gInputMode;
extern u8 gInitialisationState;
extern u8 D_802912DE;
extern u8 D_803D5538;
extern void set_game_state(void* entity, s16 state, s32 value);
extern void load_smashing_start(void);
extern void stop_all_sounds(void);

static u8 s_intro_skip_armed;
static u8 s_intro_skip_pending;
static u8 s_waiting_for_dog;
static u8 s_dog_drawn;
static u8 s_reveal_frames;

// Hooked inside the normal dog's visible model branch, after its culling and
// camera checks. Only the player dog can release the skip's black screen.
RECOMP_EXPORT void sssv_intro_skip_dog_drawn(void) {
    if (s_waiting_for_dog && D_803D5538 != 0) {
        s_dog_drawn = 1;
    }
}

// Keep this overlay separate from scripted transitions: level setup must still
// be able to finish its own fades and transition-completion waits underneath it.
void sssv_draw_intro_skip_blackout(void) {
    if (s_waiting_for_dog) {
        draw_rectangle(&gMainDL, 0, 0, 320, 240, 0, 0, 0, 255);
    } else if (s_reveal_frames != 0) {
        draw_rectangle(&gMainDL, 0, 0, 320, 240, 0, 0, 0,
            s_reveal_frames * 255 / 10);
        --s_reveal_frames;
    }
}

// Called before gameplay-frame setup so a reload cannot invalidate an already
// assembled display list. Bit 0 is the original script's "intro seen" flag.
RECOMP_EXPORT void sssv_update_intro_skip(void) {
    if (s_waiting_for_dog || s_reveal_frames != 0) {
        if (gGameState.level != 1) {
            s_waiting_for_dog = 0;
            s_dog_drawn = 0;
            s_reveal_frames = 0;
        } else if (s_waiting_for_dog && s_dog_drawn) {
            // The previous frame submitted the dog; reveal from the next frame
            // so there is no uncovered frame while the model is being prepared.
            s_waiting_for_dog = 0;
            s_dog_drawn = 0;
            s_reveal_frames = 10;
        }
        return;
    }
    const s32 active = gGameState.level == 1 && gOverlayMenuState.unk0 == 0 &&
        gInputMode == 0 && gInitialisationState == 0 &&
        (D_8023F260.script_progress & 1) == 0;
    if (!active || gControllerInput == NULL) {
        s_intro_skip_armed = 0;
        s_intro_skip_pending = 0;
        return;
    }

    if (s_intro_skip_pending) {
        if (gScreenTransition.transitionId != 3) {
            // A scripted camera cut must not replace the requested fade.
            gScreenTransition.transitionId = 3;
            gScreenTransition.unk0 = 2;
        }
        if (gScreenTransition.unk0 == 1) {
            s_intro_skip_pending = 0;
            s_intro_skip_armed = 0;
            stop_all_sounds();
            // Use the same save update and level-script branch as a completed
            // intro. This preserves the other progress bits and selected bank.
            set_game_state(NULL, 0x7FA2, D_8023F260.script_progress | 1);
            load_smashing_start();
            s_waiting_for_dog = 1;
            s_dog_drawn = 0;
            gScreenTransition.unk0 = 2;
            gScreenTransition.unk2 = 1;
            gScreenTransition.unk6 = 0;
            gScreenTransition.transitionId = 2;
            gControllerInput->button &= ~0x1000;
            D_802912DE = 0;
        }
        return;
    }

    if ((gControllerInput->button & 0x1000) == 0) {
        s_intro_skip_armed = 1;
    } else if (s_intro_skip_armed && D_802912DE == 1) {
        s_intro_skip_pending = 1;
        s_intro_skip_armed = 0;
        gScreenTransition.unk0 = 2;
        gScreenTransition.unk2 = 1;
        gScreenTransition.unk6 = 0;
        gScreenTransition.transitionId = 3;
        start_sequence_volume_fade(0, 10.0f, 20.0f, 0.0f);
        start_sfx_volume_fade(10.0f, 20.0f, 0.0f);
        // Do not also pass the skip press to the ordinary pause-menu handler.
        gControllerInput->button &= ~0x1000;
        D_802912DE = 0;
    }
}
