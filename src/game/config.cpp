#include "sssv_config.h"
#include "sssv_mirror_input.h"
#include "sssv_game.h"
#include "recompui/config.h"
#include "recompui/renderer.h"
#include "util/file.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace sssv {

namespace {
    using EnumOptionVector = const std::vector<recomp::config::ConfigOptionEnumOption>;
    constexpr const char* HDHudOptionId = "sssv_hd_hud";
    constexpr const char* HDTextOptionId = "sssv_hd_text";
    constexpr const char* HDWorldOptionId = "sssv_hd_world";
    enum class HDTextOption { Off, On };
    static EnumOptionVector hd_text_options = {
        { HDTextOption::On, "On" },
        { HDTextOption::Off, "Off" }
    };

    bool texture_pack_present() {
        std::error_code error;
        return std::filesystem::is_directory(recompui::file::get_program_path() / "Textures", error);
    }

    std::string texture_pack_heading() {
        std::ifstream file(recompui::file::get_program_path() / "Textures" / "name.txt", std::ios::binary);
        std::string name{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        if (name.starts_with("\xEF\xBB\xBF")) name.erase(0, 3);
        const auto first = name.find_first_not_of(" \t\r\n\f\v");
        if (first == std::string::npos) return "Textures";
        const auto last = name.find_last_not_of(" \t\r\n\f\v");
        return name.substr(first, last - first + 1);
    }


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

    recomp::config::Config& general_config = recompui::config::create_general_tab(general_options);
    general_config.add_enum_option(
        "sssv_mirror_mode", "Mirror Mode (Experimental)",
        "Mirrors the world horizontally while keeping HUD and text readable. Player movement and camera left/right are reversed automatically.",
        hd_text_options, HDTextOption::Off
    );
    recomp::config::Config& graphics_config = recompui::config::create_graphics_tab();
    if (texture_pack_present()) {
        recompui::config::set_option_section_heading(recompui::config::graphics::id, HDTextOptionId, texture_pack_heading());
        graphics_config.add_enum_option(
            HDTextOptionId,
            "HD Text",
            "Uses sharper in-game text. Off restores the original bitmap text.",
            hd_text_options,
            HDTextOption::On
        );
        graphics_config.add_enum_option(
            HDHudOptionId,
            "HD Textures",
            "Uses high-resolution title and menu artwork, level thumbnails, trophies, health and energy bars, and green LCD numbers. Off restores the original textures.",
            hd_text_options,
            HDTextOption::On
        );
        graphics_config.add_enum_option(
            HDWorldOptionId,
            "HD World Textures",
            "Uses high-resolution world sprites and signs. Off restores the original world textures independently of the menu and HUD artwork.",
            hd_text_options,
            HDTextOption::On
        );
    }
    recompui::config::create_controls_tab();
    recompui::config::create_sound_tab();
    recompui::config::create_mods_tab();
    recompui::config::finalize();
    recompui::renderer::set_hd_text_enabled(get_hd_text_enabled());
    recompui::renderer::set_hd_hud_enabled(get_hd_hud_enabled());
    recompui::renderer::set_hd_world_enabled(get_hd_world_enabled());
}

bool get_hd_text_enabled() {
    if (!texture_pack_present()) return false;
    try {
        auto& config = recompui::config::get_graphics_config();
        return !config.has_option(HDTextOptionId) ||
            static_cast<HDTextOption>(std::get<uint32_t>(config.get_option_value(HDTextOptionId))) == HDTextOption::On;
    } catch (const std::exception&) {
        return true;
    }
}
bool get_mirror_mode_enabled() {
    try {
        auto& config = recompui::config::get_general_config();
        return config.has_option("sssv_mirror_mode") &&
            static_cast<HDTextOption>(std::get<uint32_t>(config.get_option_value("sssv_mirror_mode"))) == HDTextOption::On;
    } catch (const std::exception&) { return false; }
}
bool get_hd_hud_enabled() {
    if (!texture_pack_present()) return false;
    try {
        auto& config = recompui::config::get_graphics_config();
        return !config.has_option(HDHudOptionId) ||
            static_cast<HDTextOption>(std::get<uint32_t>(config.get_option_value(HDHudOptionId))) == HDTextOption::On;
    } catch (const std::exception&) {
        return true;
    }
}

bool get_hd_world_enabled() {
    if (!texture_pack_present()) return false;
    try {
        auto& config = recompui::config::get_graphics_config();
        return !config.has_option(HDWorldOptionId) ||
            static_cast<HDTextOption>(std::get<uint32_t>(config.get_option_value(HDWorldOptionId))) == HDTextOption::On;
    } catch (const std::exception&) {
        return true;
    }
}

void on_init(uint8_t* rdram, recomp_context* ctx) {
    sssv::mirror::gameMemory.store(rdram, std::memory_order_relaxed);
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
