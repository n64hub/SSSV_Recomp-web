#pragma once
#include <atomic>
#include <cstdint>
#include <cstring>

namespace sssv::mirror {
    inline std::atomic<uint8_t*> gameMemory{nullptr};

    inline bool gameplay_input(const uint8_t* memory) {
        if (!memory) return false;
        const auto byte = [memory](uint32_t address) { return memory[(address & 0x7fffffu) ^ 3u]; };
        const auto half = [memory](uint32_t address) {
            uint16_t value;
            std::memcpy(&value, memory + ((address & 0x7fffffu) ^ 2u), sizeof(value));
            return value;
        };
        const auto level = half(0x803f2d38);
        const auto cameraUI = byte(0x803f2aa2);
        return level >= 1 && level <= 34 && byte(0x803f2d10) == 0 &&
            half(0x803f6680) == 0 && half(0x803e1bc0) == 0 && cameraUI != 3 && cameraUI != 4;
    }

    inline void reflect_input(uint16_t& buttons, float& x) {
        x = -x;
        constexpr uint16_t left = 0x0200 | 0x0002, right = 0x0100 | 0x0001;
        buttons = (buttons & ~(left | right)) | ((buttons & left) >> 1) | ((buttons & right) << 1);
    }
}
