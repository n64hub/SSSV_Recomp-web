#include "sssv_config.h"
#include "sssv_game.h"
#include "recompui/config.h"
#include "recompui/renderer.h"
#include "util/file.h"

#include <filesystem>
#include <string>
#include <vector>

namespace sssv {

namespace {
    using EnumOptionVector = const std::vector<recomp::config::ConfigOptionEnumOption>;
    constexpr const char* HDHudOptionId = "sssv_hd_hud";
    constexpr const char* HDTextOptionId = "sssv_hd_text";
    enum class HDTextOption { Off, On };
    static EnumOptionVector hd_text_options = {
        { HDTextOption::On, "On" },
        { HDTextOption::Off, "Off" }
    };


}

void init_config() {
    std::filesystem::path recomp_dir = recompui::file::get_app_folder_path();

    if (!recomp_dir.empty()) {
        std::filesystem::create_directories(recomp_dir);
    }

    recompui::config::GeneralTabOptions general_options{};
    general_options.has_rumble_strength = true;
    general_options.has_gyro_sensitivity = false;
    general_options.has_mouse_sensitivity = false;

    recompui::config::create_general_tab(general_options);
    recomp::config::Config& graphics_config = recompui::config::create_graphics_tab();
    graphics_config.add_enum_option(
        HDTextOptionId,
        "HD Text",
        "Uses sharper in-game text and menu header lettering. Off restores the original bitmap text and header.",
        hd_text_options,
        HDTextOption::On
    );
    graphics_config.add_enum_option(
        HDHudOptionId,
        "HD Hud",
        "Uses high-resolution health and energy bars and green LCD numbers. Off restores the original HUD artwork.",
        hd_text_options,
        HDTextOption::On
    );
    recompui::config::create_controls_tab();
    recompui::config::create_sound_tab();
    recompui::config::create_mods_tab();
    recompui::config::finalize();
    recompui::renderer::set_hd_text_enabled(get_hd_text_enabled());
    recompui::renderer::set_hd_hud_enabled(get_hd_hud_enabled());
}

bool get_hd_text_enabled() {
    try {
        auto& config = recompui::config::get_graphics_config();
        return !config.has_option(HDTextOptionId) ||
            static_cast<HDTextOption>(std::get<uint32_t>(config.get_option_value(HDTextOptionId))) == HDTextOption::On;
    } catch (const std::exception&) {
        return true;
    }
}
bool get_hd_hud_enabled() {
    try {
        auto& config = recompui::config::get_graphics_config();
        return !config.has_option(HDHudOptionId) ||
            static_cast<HDTextOption>(std::get<uint32_t>(config.get_option_value(HDHudOptionId))) == HDTextOption::On;
    } catch (const std::exception&) {
        return true;
    }
}

void on_init(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    (void)ctx;

    // librecomp clears func_map during init_overlays(), so absolute 0x8Fxxxxxx patch
    // bridge symbols must be restored here before the game starts executing.
    restore_runtime_patch_symbols();
}

std::string get_game_thread_name(const OSThread* t) {
    std::string name = "[Game] ";

    switch (t->id) {
        case 0:
            switch (t->priority) {
                case 150:
                    name += "PIMGR";
                    break;
                case 80:
                    name += "VIMGR";
                    break;
                default:
                    name += std::to_string(t->id);
                    break;
            }
            break;
        case 1:
            name += "IDLE";
            break;
        case 3:
            name += "MAIN";
            break;
        case 4:
            name += "AUDIO";
            break;
        case 5:
            name += "SCHED";
            break;
        case 6:
            name += "GRAPH";
            break;
        case 7:
            name += "RMON";
            break;
        default:
            name += std::to_string(t->id);
            break;
    }

    return name;
}

} // namespace sssv
