#include "sssv_patch_common.h"
#include "sssv_render_context.h"
#include "sssv_rt64_tagging.h"
#include "sssv_patch_trace.h"
#include "sssv_patch_ui_domain.h"

static u32 g_dbg_osd_score_calls = 0;
static u32 g_dbg_osd_timer_calls = 0;
static u32 g_dbg_osd_text_calls = 0;
static u32 g_dbg_credits_calls = 0;

// The textless HD image retains the original frame and decorations. Its caption
// is drawn with actual glyphs, using the same font and layout as other text.
extern u8* sssv_begin_hd_header_font(void);
extern void sssv_end_hd_header_font(u8* original);

RECOMP_EXPORT void sssv_draw_hd_header(void) {
    s16 text[32];
    u8* original_bitmap;
    load_default_display_list(&gMainDL);
    select_font(0, 0, 0, 0);
    original_bitmap = sssv_begin_hd_header_font();
    set_menu_text_color(255, 255, 0, 255);
    prepare_text((u8*)"SPACE STATION SILICON VALLEY", text);
    // A slightly shorter caption leaves balanced breathing room inside the
    // blue border without changing the approved horizontal placement.
    rc_ensure_rt64_extended_enabled(&gMainDL);
    gEXSetRectAlign(gMainDL++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER,
        -160 * 4, 0, -160 * 4, 0);
    display_text_centered(&gMainDL, text, 120, 19, 11.0f, 9.0f);
    sssv_end_hd_header_font(original_bitmap);
    gEXSetRectAlign(gMainDL++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER,
        -160 * 4, 0, -160 * 4, 0);
    gDPPipeSync(gMainDL++);
}

// Rectangles keep their native pixel proportions; only their anchor moves in
// Expand mode. The camera and background retain the selected aspect ratio.
static void set_hud_anchor(s32 origin, s16 offset) {
    const f32 window_aspect = recomp_get_target_aspect_ratio(SSSV_BASE_ASPECT);
    if (window_aspect <= SSSV_BASE_ASPECT + 0.000001f) {
        return;
    }
    if (origin == G_EX_ORIGIN_LEFT || origin == G_EX_ORIGIN_RIGHT) {
        const f32 inset = (window_aspect - recomp_get_hud_aspect_ratio()) * 240.0f * 2.0f;
        const s16 inset_quarters = (s16)MIN(inset + 0.5f, 30000.0f);
        offset += (origin == G_EX_ORIGIN_LEFT) ? inset_quarters : -inset_quarters;
    }
    rc_ensure_rt64_extended_enabled(&gMainDL);
    gEXSetRectAlign(gMainDL++, origin, origin, offset, 0, offset, 0);
    gEXSetScissorAlign(gMainDL++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_RIGHT,
        0, 0, -320, 0, 0, 0, 320, 240);
    gDPSetScissor(gMainDL++, G_SC_NON_INTERLACE, 0, 0, 320, 240);
}

RECOMP_EXPORT void sssv_hud_left_begin(void) {
    set_hud_anchor(G_EX_ORIGIN_LEFT, 0);
}

RECOMP_EXPORT void sssv_hud_layout_end(void) {
    if (recomp_get_target_aspect_ratio(SSSV_BASE_ASPECT) <= SSSV_BASE_ASPECT + 0.000001f) {
        return;
    }
    rc_ensure_rt64_extended_enabled(&gMainDL);
    gEXSetRectAlign(gMainDL++, G_EX_ORIGIN_NONE, G_EX_ORIGIN_NONE, 0, 0, 0, 0);
    gEXSetScissorAlign(gMainDL++, G_EX_ORIGIN_NONE, G_EX_ORIGIN_NONE,
        0, 0, 0, 0, 0, 0, 4095, 4095);
    gDPSetScissor(gMainDL++, G_SC_NON_INTERLACE, 0, 0,
        compute_target_width_for_height_patch(get_safe_screen_height()), get_safe_screen_height());
}

static inline u32 begin_hud_pass(void) {
    u32 hud_id = rc_next_hud_transform_id();

    rc_set(RC_HUD_ORTHO);
    rc_note_texrect_context(TRC_HUD_UI, 0x55490001u);
    cur_perspective_projection_transform_id = SSSV_PROJECTION_HUD_TRANSFORM_ID;
    cur_ortho_projection_transform_id = SSSV_PROJECTION_HUD_TRANSFORM_ID;

    rt64_tag_projection_matrix(&gMainDL, SSSV_PROJECTION_HUD_TRANSFORM_ID);
    rt64_tag_model_matrix(&gMainDL, hud_id, 0);
    set_hud_anchor(G_EX_ORIGIN_CENTER, -160 * 4);
    return hud_id;
}

static inline void end_hud_pass(void) {
    rt64_pop_model_matrix(&gMainDL);
    sssv_hud_layout_end();
}

RECOMP_PATCH void osd_draw_score(void) {
    const u8 font_lcd = 1;
    const u32 hud_id = begin_hud_pass();
    set_hud_anchor(G_EX_ORIGIN_RIGHT, -320 * 4);
    sprintf((char*)gGameState.scoreText, "%8d", gGameState.score);
    select_font(0, font_lcd, 1, 0);
    display_score(&gMainDL, (u8*)gGameState.scoreText, ui_safe_right_anchor(34), (u16)((gHudBarBaseY >> 2) - 10));
    end_hud_pass();

    g_dbg_osd_score_calls++;
    if (SHOULD_TRACE_PERIODIC(g_dbg_osd_score_calls, 8, 240)) {
        PATCH_TRACE(PATCH_TAG_OSD_SCORE, g_dbg_osd_score_calls, (u16)ui_safe_width(), (u16)hud_id);
    }
}

RECOMP_PATCH void osd_draw_timer(s16 arg0) {
    s16 text[20];
    const u32 hud_id = begin_hud_pass();

    if (gHudTimerSeconds > 0) {
        if (gHudTimerSeconds < 60) {
            sprintf((char*)gHudTimerAscii, "%d", gHudTimerSeconds);
        } else {
            sprintf((char*)gHudTimerAscii, "%d:%02d", gHudTimerSeconds / 60, gHudTimerSeconds % 60);
        }
    }

    prepare_text((u8*)gHudTimerAscii, text);
    load_default_display_list(&gMainDL);
    set_menu_text_color(255, 255, 0, 255);
    select_font(0, 0, 1, 0);
    display_text_centered(&gMainDL, text, ui_safe_center_x(), (u16)arg0, 16.0f, 16.0f);
    end_hud_pass();

    g_dbg_osd_timer_calls++;
    if (SHOULD_TRACE_PERIODIC(g_dbg_osd_timer_calls, 8, 240)) {
        PATCH_TRACE(PATCH_TAG_OSD_TIMER, g_dbg_osd_timer_calls, (u16)ui_safe_center_x(), (u16)hud_id);
    }
}

RECOMP_PATCH void draw_hud_center_text(s16 arg0) {
    s16 saved_width;
    const u32 hud_id = begin_hud_pass();
    load_default_display_list(&gMainDL);
    set_menu_text_color(255, 255, 0, 255);
    select_font(0, 0, 1, 0);
    // Restore native width: display_text_word_wrapped derives its word-wrap threshold from gScreenWidth.
    saved_width = gScreenWidth;
    gScreenWidth = SSSV_BASE_WIDTH;
    display_text_word_wrapped(&gMainDL, gHudCenterText, ui_safe_center_x(), (u16)arg0, 16.0f, 16.0f, 0x10);
    gScreenWidth = saved_width;
    end_hud_pass();

    g_dbg_osd_text_calls++;
    if (SHOULD_TRACE_PERIODIC(g_dbg_osd_text_calls, 8, 240)) {
        PATCH_TRACE(PATCH_TAG_OSD_TEXT, g_dbg_osd_text_calls, (u16)ui_safe_center_x(), (u16)hud_id);
    }
}

// RDP text rectangles have no interpolated vertex positions. Reuse the native
// font layout/texture loads, replacing only their rectangles with RSP quads.
// Each credit entry retains its own ID when the 16-line window rolls forward.
static Vtx s_credits_vertices[3][4096] __attribute__((aligned(8)));
static Mtx s_credits_projection;
static Mtx s_credits_identity;
static VpRaw s_credits_viewport __attribute__((aligned(8)));
#define CREDITS_VERTEX_CAPACITY 4096
static Vtx* credits_vertices(void) { return s_credits_vertices[rc_frame_index() % 3]; }

static void credits_matrix(Mtx* target, const f32* source) {
    u32* packed = (u32*)target;
    s32 i;
    for (i = 0; i < 8; i++) {
        const s32 a = (s32)(source[i * 2] * 65536.0f);
        const s32 b = (s32)(source[i * 2 + 1] * 65536.0f);
        packed[i] = ((u32)a & 0xFFFF0000u) | (((u32)b >> 16) & 0xFFFFu);
        packed[i + 8] = ((u32)a << 16) | ((u32)b & 0xFFFFu);
    }
}

static void credits_begin_quads(void) {
    Mtx* projection_mtx = &s_credits_projection;
    Mtx* identity_mtx = &s_credits_identity;
    VpRaw* viewport = &s_credits_viewport;
    const f32 projection[16] = {
        1.0f / 160.0f, 0, 0, 0, 0, -1.0f / 120.0f, 0, 0,
        0, 0, -1, 0, -1, 1, 0, 1
    };
    const f32 identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    credits_matrix(projection_mtx, projection);
    credits_matrix(identity_mtx, identity);
    viewport->vscale[0] = viewport->vtrans[0] = 640;
    viewport->vscale[1] = viewport->vtrans[1] = 480;
    viewport->vscale[2] = viewport->vtrans[2] = 511;
    viewport->vscale[3] = viewport->vtrans[3] = 0;
    gEXSetRDRAMExtended(gMainDL++, 1);
    gEXSetViewportAlign(gMainDL++, G_EX_ORIGIN_CENTER, -160 * 4, 0);
    gSPViewport(gMainDL++, viewport);
    rt64_tag_projection_matrix(&gMainDL, 0x00001014u);
    gSPMatrix(gMainDL++, projection_mtx, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(gMainDL++, identity_mtx, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH);
    // F3D G_CLEARGEOMETRYMODE: Z, culling, fog, lighting and generated UVs.
    gImmp1_(gMainDL++, 0xB6, 0x000F3001u);
    // RSP triangles need perspective texture coordinates even with an
    // orthographic projection; the rectangle-only affine mode halves UVs.
    gDPSetTexturePersp(gMainDL++, G_TP_PERSP);
    gSPTexture(gMainDL++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
}

static void credits_convert_rects(Gfx* first, Gfx* end, Vtx* vertices, s32* count) {
    Gfx* command;
    for (command = first; command + 2 < end; command++) {
        const u32 w0 = command->words.w0;
        const u32 w1 = command->words.w1;
        s32 x0, y0, x1, y1, j;
        Vtx* quad;
        if ((w0 >> 24) != G_TEXRECT || *count + 4 > CREDITS_VERTEX_CAPACITY) continue;
        x0 = (w1 >> 12) & 0xFFF; y0 = w1 & 0xFFF;
        x1 = (w0 >> 12) & 0xFFF; y1 = w0 & 0xFFF;
        if (y0 & 0x800) y0 -= 0x1000;
        if (y1 & 0x800) y1 -= 0x1000;
        quad = &vertices[*count];
        *count += 4;
        for (j = 0; j < 4; j++) {
            quad[j].v.ob[0] = ((j & 1) ? x1 : x0) / 4;
            quad[j].v.ob[1] = ((j & 2) ? y1 : y0) / 4;
            quad[j].v.ob[2] = 0;
            quad[j].v.flag = 0;
            quad[j].v.tc[0] = (j & 1) ? 16 * 32 : 0;
            quad[j].v.tc[1] = (j & 2) ? 16 * 32 : 0;
            quad[j].v.cn[0] = quad[j].v.cn[1] = quad[j].v.cn[2] = quad[j].v.cn[3] = 255;
        }
        gSPVertex(command, quad, 4, 0);
        gSP1Triangle(command + 1, 0, 1, 2, 0);
        gSP1Triangle(command + 2, 2, 1, 3, 0);
        command += 2;
    }
}

RECOMP_PATCH s32 display_credits(void) {
    s16 i;
    s16 wide[58];
    u8 ascii[64];
    const u16 center_x = ui_safe_center_x();
    const u32 hud_id = begin_hud_pass();
    Vtx* vertices = credits_vertices();
    s32 vertex_count = 0;

    load_default_display_list(&gMainDL);
    set_menu_text_color(0xFF, 0xFF, 0xFF, 0xFF);
    select_font(0, 0, 0, 0);
    credits_begin_quads();

    for (i = 0; i < 16; i++) {
        sprintf((char*)ascii, "%s", gCreditsLines[i + gCreditsEntryOffset]);
        prepare_text(ascii, wide);
        if ((ascii[0] == 'E') && (ascii[1] == 'N') && (ascii[2] == 'D')) {
            gCreditsEntryOffset = 0;
            i = 1000;
        } else {
            Gfx* first;
            rt64_tag_model_matrix(&gMainDL, 0x10010000u + gCreditsEntryOffset + i, 0);
            // A group tag alone does not create a new RSP transform. Each line
            // needs a matrix load so rolling the text window preserves matches.
            gSPMatrix(gMainDL++, &s_credits_identity, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
            first = gMainDL;
            display_text_centered(&gMainDL, wide, center_x, (u16)(gCreditsVerticalPosition + (i * 16)), 16.0f, 16.0f);
            credits_convert_rects(first, gMainDL, vertices, &vertex_count);
            rt64_pop_model_matrix(&gMainDL);
        }
    }

    gCreditsVerticalPosition -= 1;
    if (gCreditsVerticalPosition == -16) {
        gCreditsEntryOffset += 1;
        gCreditsVerticalPosition = 0;
    }
    gSPPopMatrix(gMainDL++, G_MTX_MODELVIEW);
    gEXSetRDRAMExtended(gMainDL++, 0);
    gEXSetViewportAlign(gMainDL++, G_EX_ORIGIN_NONE, 0, 0);
    gSPViewport(gMainDL++, &gMainViewport);
    end_hud_pass();

    g_dbg_credits_calls++;
    if (SHOULD_TRACE_PERIODIC(g_dbg_credits_calls, 4, 240)) {
        PATCH_TRACE(PATCH_TAG_CREDITS, g_dbg_credits_calls, center_x, (u16)hud_id);
    }

    if (i > 100) {
        return 1;
    }
    return 0;
}

RECOMP_PATCH void check_cheats(OSContPad *contPad) {
    u8* cheat_input_buffer = CHEAT_INPUT_BUFFER_ADDR;
    u16 lastButton;
    u16 butDown;
    u32 butDown2;
    s16 text[60];
    const u32 hud_id = rc_next_hud_transform_id();

    rc_set(RC_HUD_ORTHO);
    rc_note_texrect_context(TRC_HUD_UI, 0xCEA70000u);
    cur_perspective_projection_transform_id = SSSV_PROJECTION_HUD_TRANSFORM_ID;
    cur_ortho_projection_transform_id = SSSV_PROJECTION_HUD_TRANSFORM_ID;
    rt64_tag_projection_matrix(&gMainDL, SSSV_PROJECTION_HUD_TRANSFORM_ID);

    if (gCheats.debugMode != 0) {
        set_hud_anchor(G_EX_ORIGIN_CENTER, -160 * 4);
        load_default_display_list(&gMainDL);
        set_menu_text_color(0xFF, 0xFF, 0, 0xFF);
        select_font(0, 0, 0, 0);
        if ((gCheats.debugMode != 0) && (gOverlayMenuState.unk0 == 0)) {
            sprintf(
                gDebugTextBuffer,
                "(%3d  %3d  %4d)",
                gAnimalState.animals[gCurrentAnimalIndex].animal->position.xPos.h >> 6,
                gAnimalState.animals[gCurrentAnimalIndex].animal->position.zPos.h >> 6,
                gAnimalState.animals[gCurrentAnimalIndex].animal->position.yPos.h
            );
            prepare_text((u8*)gDebugTextBuffer, text);
            display_text(&gMainDL, text, ui_safe_right_anchor(20), 20, 16.0f, 16.0f);
            prepare_text((u8*)"Ver - 1.37", text);
            display_text(&gMainDL, text, ui_safe_right_anchor(20), 36, 16.0f, 16.0f);
        }
        sssv_hud_layout_end();
    }

    lastButton = 0;
    if (gCheatInputDebounceTimer != 0) {
        gCheatInputDebounceTimer--;
    }

    butDown = contPad->button;
    butDown2 = butDown;
    if ((gCheatInputNeutralLatch == 0) && (butDown2 == 0)) {
        gCheatInputNeutralLatch = 1;
        return;
    } else if (butDown2 != 0) {
        gCheatInputNeutralLatch = 0;
    }

    if ((gCheatInputDebounceTimer == 0) && (butDown2 != 0) && (gCheatLastButtonCode != butDown)) {
        u16 down = butDown;
        if (gFrameStepDivisor == 1) {
            gCheatInputDebounceTimer = 12;
        } else {
            gCheatInputDebounceTimer = 6;
        }
        if (down & CONT_A) {
            lastButton = 'A';
        }
        if (down & CONT_B) {
            lastButton = 'B';
        }
        if (down & CONT_UP) {
            lastButton = 'U';
        }
        if (down & CONT_DOWN) {
            lastButton = 'D';
        }
        if (down & CONT_LEFT) {
            lastButton = 'L';
        }
        if (down & CONT_RIGHT) {
            lastButton = 'R';
        }
        if (down & U_CBUTTONS) {
            lastButton = 'N';
        }
        if (down & D_CBUTTONS) {
            lastButton = 'S';
        }
        if (down & L_CBUTTONS) {
            lastButton = 'W';
        }
        if (down & R_CBUTTONS) {
            lastButton = 'E';
        }
        if (down & Z_TRIG) {
            lastButton = 'Z';
        }
        if (down & L_TRIG) {
            lastButton = 'I';
        }
        if (down & R_TRIG) {
            lastButton = 'T';
        }
        gCheatLastButtonCode = (u8)lastButton;
    }

    if (lastButton != 0) {
        if (++gCheatInputCursor >= 20) {
            gCheatInputCursor = 0;
        }
        cheat_input_buffer[gCheatInputCursor] = (u8)lastButton;
    }

    if (check_cheat_code(cheat_input_buffer, "WIZDIZWE")) {
        gAnimalState.animals[gCurrentAnimalIndex].animal->health = 127;
        play_sound_effect(SFX_CHEAT_ENABLED, 0, 0x5000, 1.0f, 64);
        gCheatInputCursor = 0;
    }
    if (check_cheat_code(cheat_input_buffer, "UDIZDUZD")) {
        play_sound_effect(SFX_CHEAT_ENABLED, 0, 0x5000, 1.0f, 64);
        gCheats.europe = 1 - gCheats.europe;
        gCheatInputCursor = 0;
    }
    if (check_cheat_code(cheat_input_buffer, "UDZIDEZD")) {
        play_sound_effect(SFX_CHEAT_ENABLED, 0, 0x5000, 1.0f, 64);
        gCheats.ice = 1 - gCheats.ice;
        gCheatInputCursor = 0;
    }
    if (check_cheat_code(cheat_input_buffer, "UDIZDWZD")) {
        play_sound_effect(SFX_CHEAT_ENABLED, 0, 0x5000, 1.0f, 64);
        gCheats.jungle = 1 - gCheats.jungle;
        gCheatInputCursor = 0;
    }
    if (check_cheat_code(cheat_input_buffer, "UDIZDLZD")) {
        play_sound_effect(SFX_CHEAT_ENABLED, 0, 0x5000, 1.0f, 64);
        gCheats.desert = 1 - gCheats.desert;
        gCheatInputCursor = 0;
    }
    if (check_cheat_code(cheat_input_buffer, "UDIZDRZD")) {
        play_sound_effect(SFX_CHEAT_ENABLED, 0, 0x5000, 1.0f, 64);
        gCheats.bcp = 1 - gCheats.bcp;
        gCheatInputCursor = 0;
    }
    if (check_cheat_code(cheat_input_buffer, "DUZIDLZD")) {
        play_sound_effect(SFX_CHEAT_ENABLED, 0, 0x5000, 1.0f, 64);
        gCheats.hidden = 1 - gCheats.hidden;
        gCheatInputCursor = 0;
    }
    if (check_cheat_code(cheat_input_buffer, "UIZDLZDU")) {
        play_sound_effect(SFX_CHEAT_ENABLED, 0, 0x5000, 1.0f, 64);
        cheat_activate_scale_pulse_effect();
        gCheatInputCursor = 0;
    }
    if (check_cheat_code(cheat_input_buffer, "IDZIDUIL")) {
        play_sound_effect(SFX_CHEAT_ENABLED, 0, 0x5000, 1.0f, 64);
        cheat_activate_energy_camera_effect();
        gCheatInputCursor = 0;
    }
    if (check_cheat_code(cheat_input_buffer, "ZDUIRILR")) {
        play_sound_effect(SFX_CHEAT_ENABLED, 0, 0x5000, 1.0f, 64);
        cheat_activate_big_head_effect();
        gCheatInputCursor = 0;
    }
    if (check_cheat_code(cheat_input_buffer, "LRZILZRL")) {
        play_sound_effect(SFX_CHEAT_ENABLED, 0, 0x5000, 1.0f, 64);
        cheat_toggle_mystery_bear();
        gCheatInputCursor = 0;
    }
    if (check_cheat_code(cheat_input_buffer, "DANISIL\0")) {
        play_sound_effect(SFX_CHEAT_ENABLED, 0, 0x5000, 1.0f, 64);
        cheat_clear_camera_effects();
        gCheatInputCursor = 0;
    }
    PATCH_TRACE(PATCH_TAG_OSD_TEXT, 0xCEA70000u, hud_id, (u16)ui_safe_width());
}

// gScreenWidth/2 becomes 213 in widescreen; ui_safe_center_x() keeps the message centered.
RECOMP_PATCH void no_controller_message(void) {
    if (gControllerConnected == 0) {
        load_segments(&gMainDL, gDisplayListContext);
        draw_rectangle(&gMainDL, 0, 16, 320, 36, 40, 40, 40, 128);
        gDPPipeSync(gMainDL++);
        set_hud_anchor(G_EX_ORIGIN_CENTER, -160 * 4);

        load_default_display_list(&gMainDL);
        select_font(0, 0 /* FONT_DEFAULT */, 0, 0);
        set_menu_text_color(0xFF, 0xFF, 0xFF, 0xFF);
        display_text_centered(&gMainDL, gNoControllerMessageText, ui_safe_center_x(), 20, 16.0f, 16.0f);
        gDPPipeSync(gMainDL++);
        sssv_hud_layout_end();
    }
}
