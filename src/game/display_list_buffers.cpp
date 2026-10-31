#include <array>
#include <cstdint>
#include <fstream>
#include <stdexcept>

#include "recomp.h"
#include "librecomp/addresses.hpp"

namespace {
constexpr uint32_t FrameContexts = 0x80162658;
constexpr uint32_t FrameContextSize = 0x3BBF0;
constexpr uint32_t DisplayListOffset = 0x4E0;
constexpr int32_t MainDisplayListHead = static_cast<int32_t>(0x801D9E7C);
constexpr uint32_t CommandCapacity = 16384;
constexpr uint32_t BufferBytes = CommandCapacity * 8;
std::array<uint32_t, 2> buffers{};
uint32_t highWater = 0;

size_t frame_index(uint32_t frame) {
    const uint32_t offset = frame - FrameContexts;
    if (offset % FrameContextSize != 0 || offset / FrameContextSize >= buffers.size()) {
        throw std::runtime_error("Unexpected graphics frame context.");
    }
    return offset / FrameContextSize;
}

gpr vram(uint32_t address) {
    return static_cast<int64_t>(static_cast<int32_t>(address));
}
}

// Runs after thread7 initializes the original display-list heads. Preserve the
// original frame/segment layout, but give each buffered frame its own larger
// main list. Widescreen subtitle drawing can exceed the original 4000 commands
// and overwrite the adjacent layer list (including its end command).
extern "C" void sssv_begin_main_display_list(uint8_t* rdram, recomp_context* ctx) noexcept(false) {
    auto& buffer = buffers[frame_index(static_cast<uint32_t>(ctx->r4))];
    if (buffer == 0) {
        auto* allocation = static_cast<uint8_t*>(recomp::alloc(rdram, BufferBytes));
        if (!allocation) throw std::runtime_error("Could not allocate the main display-list buffer.");
        const auto offset = static_cast<uint64_t>(allocation - rdram);
        if (offset + BufferBytes > 0x4000000) {
            throw std::runtime_error("Display-list buffer exceeds the graphics address range.");
        }
        buffer = static_cast<uint32_t>(offset) | 0x80000000;
    }
    MEM_W(0, MainDisplayListHead) = vram(buffer);
}

// The original caller computes size relative to the embedded mainDL. Replace
// both task arguments before func_8013107C resets gMainDL and queues the task.
extern "C" void sssv_submit_main_display_list(uint8_t* rdram, recomp_context* ctx) noexcept(false) {
    const uint32_t frame = static_cast<uint32_t>(ctx->r4);
    if (static_cast<uint32_t>(ctx->r5) != frame + DisplayListOffset) return;
    const uint32_t buffer = buffers[frame_index(frame)];
    const uint32_t end = static_cast<uint32_t>(MEM_W(0, MainDisplayListHead));
    const uint32_t bytes = end - buffer;
    if (buffer == 0 || bytes > BufferBytes || bytes % 8 != 0) {
        throw std::runtime_error("Main display-list buffer capacity exceeded.");
    }
    ctx->r5 = vram(buffer);
    ctx->r6 = bytes;
    if (bytes / 8 > highWater) {
        highWater = bytes / 8;
        if (highWater > 4000) {
            static std::ofstream log("display-list-capacity.log", std::ios::app);
            log << "main_display_list_commands=" << highWater << "; capacity=" << CommandCapacity << std::endl;
        }
    }
}
