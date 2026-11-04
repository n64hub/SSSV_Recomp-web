#pragma once
#include <cstdint>
namespace sssv {
// Terrain cells are 64 world units. Fade across the fifth cell.
constexpr uint8_t reflection_opacity(int distance) {
    return distance <= 256 ? 255 : distance >= 320 ? 0 :
        static_cast<uint8_t>(((320 - distance) * 255 + 32) / 64);
}
}
