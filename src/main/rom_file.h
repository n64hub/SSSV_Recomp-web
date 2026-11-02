#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>
#define XXH_INLINE_ALL
#include "xxhash.h"

namespace sssv {
inline std::optional<std::filesystem::path> explicit_rom_path(int argc, char** argv) {
    std::optional<std::filesystem::path> result;
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) != "--rom") continue;
        if (result || i + 1 == argc || !argv[i + 1][0] || std::string_view(argv[i + 1]).starts_with("--"))
            throw std::runtime_error("Use --rom followed by one ROM file path, for example --rom \"C:\\Games\\sssv.z64\".");
        std::error_code error;
        result = std::filesystem::absolute(std::filesystem::u8path(argv[++i]), error);
        if (error) throw std::runtime_error("Could not resolve the ROM path supplied with --rom.");
    }
    return result;
}
inline bool read_supported_rom(const std::filesystem::path& path, uint64_t hash, std::vector<uint8_t>& data) {
    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error)) return false;
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    data.resize(8 * 1024 * 1024);
    return file && file.tellg() == static_cast<std::streamoff>(data.size()) &&
        file.seekg(0) && file.read(reinterpret_cast<char*>(data.data()), data.size()) &&
        XXH3_64bits(data.data(), data.size()) == hash;
}
}
