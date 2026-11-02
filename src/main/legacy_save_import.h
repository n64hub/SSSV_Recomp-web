#pragma once
#include <filesystem>
#include <string>
#include <system_error>

namespace sssv {
enum class LegacySaveImport { NoLegacySave, ExistingPortableSave, Imported, InvalidLegacySave, IOError };

inline bool is_save_data_file(const std::filesystem::path& path) {
    auto name = path.filename().u8string();
    for (auto& letter : name) if (letter >= u8'A' && letter <= u8'Z') letter += u8'a' - u8'A';
    return name.ends_with(u8".bin") || name.ends_with(u8".bin.bak") || name.ends_with(u8".bin.temp");
}

inline LegacySaveImport import_legacy_save(const std::filesystem::path& portable_folder,
                                           const std::filesystem::path& legacy_folder,
                                           const std::filesystem::path& filename) {
    // Preserve every existing bank, mod save and runtime backup. A directory
    // alone is not save data; unrelated files such as README.txt do not block.
    std::error_code error;
    const bool portable_exists = std::filesystem::exists(portable_folder, error);
    if (error) return LegacySaveImport::IOError;
    if (portable_exists) {
        auto it = std::filesystem::recursive_directory_iterator(portable_folder, error);
        if (error) return LegacySaveImport::IOError;
        for (const auto end = std::filesystem::recursive_directory_iterator(); it != end; it.increment(error)) {
            if (error) return LegacySaveImport::IOError;
            const bool regular = it->is_regular_file(error);
            if (error) return LegacySaveImport::IOError;
            if (regular && is_save_data_file(it->path())) return LegacySaveImport::ExistingPortableSave;
        }
        if (error) return LegacySaveImport::IOError;
    }
    // Only import this game's actual 4-kbit EEPROM file, never arbitrary files.
    const auto source = legacy_folder / filename;
    const bool source_exists = std::filesystem::exists(source, error);
    if (error) return LegacySaveImport::IOError;
    if (!source_exists || !std::filesystem::is_regular_file(source, error)) return LegacySaveImport::NoLegacySave;
    if (error) return LegacySaveImport::IOError;
    if (std::filesystem::file_size(source, error) != 512 || error) return LegacySaveImport::InvalidLegacySave;
    std::filesystem::create_directories(portable_folder, error);
    if (error) return LegacySaveImport::IOError;
    // copy_options::none also prevents overwriting if another process creates
    // the destination between the directory check and this copy.
    const bool copied = std::filesystem::copy_file(source, portable_folder / filename,
                                                   std::filesystem::copy_options::none, error);
    return copied ? LegacySaveImport::Imported : LegacySaveImport::IOError;
}
}
