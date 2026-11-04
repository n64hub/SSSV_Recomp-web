#include <array>
#include <cstdint>
#include <fstream>
#include <vector>
#include <algorithm>
#include <stdexcept>

#include "recomp.h"
#include "sssv_reflection_fade.h"
#include "funcs.h"
#include "librecomp/addresses.hpp"
#include "../../lib/rt64/include/rt64_extended_gbi.h"

namespace {
constexpr uint32_t FrameContexts = 0x80162658;
constexpr uint32_t FrameContextSize = 0x3BBF0;
constexpr uint32_t DisplayListOffset = 0x4E0;
constexpr int32_t MainDisplayListHead = static_cast<int32_t>(0x801D9E7C);
constexpr uint32_t CommandCapacity = 16384;
constexpr uint32_t BufferBytes = CommandCapacity * 8;
std::array<uint32_t, 2> buffers{};
uint32_t highWater = 0;
constexpr uint32_t OpaqueDisplayListOffset = 0x9600;
std::array<uint32_t, 2> opaqueBuffers{};
uint32_t opaqueHighWater = 0;

constexpr int32_t DisplayListContextPointer = static_cast<int32_t>(0x80204278);
constexpr uint32_t ModelMatrixOffset = 0x33590;
constexpr uint32_t ModelMatrixCountOffset = 0x38918;
constexpr uint32_t MatrixBytes = 64;
constexpr uint32_t TailMatrixCapacity = 2048;
constexpr uint32_t TailMatrixBufferBytes = TailMatrixCapacity * MatrixBytes;
constexpr int32_t TailCountAddress = static_cast<int32_t>(0x803E1B02);
constexpr int32_t OpaqueDisplayListHead = static_cast<int32_t>(0x801D9E88);
struct TailMatrixBuffer {
    uint32_t address = 0;
    uint32_t used = 0;
};
std::array<TailMatrixBuffer, 2> tailBuffers{};
int activeTailPass = -1;
uint32_t tailHighWater = 0;

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

void sssv_reset_reflection_ranges();

// Runs after thread7 initializes the original display-list heads. Preserve the
// original frame/segment layout, but give each buffered frame its own larger
// main list. Widescreen subtitle drawing can exceed the original 4000 commands
// and overwrite the adjacent layer list (including its end command).
extern "C" void sssv_begin_main_display_list(uint8_t* rdram, recomp_context* ctx) noexcept(false) {
    const auto index = frame_index(static_cast<uint32_t>(ctx->r4));
    auto& buffer = buffers[index];
    tailBuffers[index].used = 0;
    sssv_reset_reflection_ranges();
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

    // Dense animal groups can also exceed the embedded opaque list's 1200
    // commands and overwrite the translucent list beside it. Keep every
    // original caller working through a wrapper at the embedded entry point.
    auto& opaqueBuffer = opaqueBuffers[index];
    if (opaqueBuffer == 0) {
        auto* allocation = static_cast<uint8_t*>(recomp::alloc(rdram, BufferBytes));
        if (!allocation) throw std::runtime_error("Could not allocate the opaque display-list buffer.");
        const auto offset = static_cast<uint64_t>(allocation - rdram);
        if (offset + BufferBytes > 0x4000000) {
            throw std::runtime_error("Opaque display-list buffer exceeds the graphics address range.");
        }
        opaqueBuffer = static_cast<uint32_t>(offset) | 0x80000000;
    }
    MEM_W(0, OpaqueDisplayListHead) = vram(opaqueBuffer);
    MEM_W(0, vram(opaqueBuffer)) = 0xB8000000; // Safe empty F3DEX list.
    MEM_W(4, vram(opaqueBuffer)) = 0;
    const uint32_t opaqueEntry = static_cast<uint32_t>(ctx->r4) + DisplayListOffset + OpaqueDisplayListOffset;
    // Scope extended RDRAM to this pass. Legacy billboards deliberately use
    // 0x81xxxxxx aliases of segment-1 textures; leaving extended mode enabled
    // would decode those aliases as heap addresses and corrupt energy sprites.
    MEM_W(0, vram(opaqueEntry)) = (RT64_HOOK_OPCODE << 24) | RT64_HOOK_MAGIC_NUMBER;
    MEM_W(4, vram(opaqueEntry)) = (RT64_HOOK_OP_ENABLE << 28) | RT64_EXTENDED_OPCODE;
    MEM_W(8, vram(opaqueEntry)) = (RT64_EXTENDED_OPCODE << 24) | G_EX_SETRDRAMEXTENDED_V1;
    MEM_W(12, vram(opaqueEntry)) = 1;
    MEM_W(16, vram(opaqueEntry)) = 0x06000000; // F3DEX G_DL, G_DL_PUSH.
    MEM_W(20, vram(opaqueEntry)) = vram(opaqueBuffer);
    MEM_W(24, vram(opaqueEntry)) = (RT64_EXTENDED_OPCODE << 24) | G_EX_SETRDRAMEXTENDED_V1;
    MEM_W(28, vram(opaqueEntry)) = 0;
    MEM_W(32, vram(opaqueEntry)) = 0xB8000000; // Return to the original caller.
    MEM_W(36, vram(opaqueEntry)) = 0;
}

// Dynamic tails do not apply the body renderer's 250-matrix capacity check.
// Sting in the Tail needs about 60 tail matrices after its animal bodies have
// already filled that pool. The overflow replaces the adjacent world camera
// and projection matrices, making the entire level disappear.
//
// These two native passes use the counter only as a full 32-bit matrix index.
// Temporarily rebase it so their existing address arithmetic writes a separate
// per-frame arena. The ordinary pool count is restored even if rendering throws.
extern "C" int sssv_render_dynamic_tail_pass(uint8_t* rdram, recomp_context* ctx, int pass) noexcept(false) {
    if (pass < 0 || pass > 1) throw std::runtime_error("Invalid dynamic-tail render pass.");
    if (activeTailPass == pass) return 0; // Re-enter the original native function.
    if (activeTailPass != -1) throw std::runtime_error("Nested dynamic-tail render passes.");

    const uint32_t display = static_cast<uint32_t>(MEM_W(0, DisplayListContextPointer));
    auto& buffer = tailBuffers[frame_index(display - DisplayListOffset)];
    const uint32_t nativeBase = display + ModelMatrixOffset;
    const int tailCount = MEM_H(0, TailCountAddress);
    if (tailCount < 0 || tailCount > 120) throw std::runtime_error("Invalid dynamic-tail count.");

    // The ribbon pass uses at most one matrix per tail; the model pass at most
    // six (a scorpion). Reserve the worst case before any native writes occur.
    const uint32_t maximumWrites = static_cast<uint32_t>(tailCount) * (pass == 0 ? 1 : 6);
    if (buffer.used + maximumWrites > TailMatrixCapacity) {
        throw std::runtime_error("Dynamic-tail matrix buffer capacity exceeded.");
    }
    if (buffer.address == 0) {
        auto* allocation = static_cast<uint8_t*>(recomp::alloc(rdram, TailMatrixBufferBytes + MatrixBytes - 1));
        if (!allocation) throw std::runtime_error("Could not allocate dynamic-tail matrices.");
        const auto offset = static_cast<uint64_t>(allocation - rdram);
        // The embedded matrix pool is 8-byte aligned, not necessarily 64-byte
        // aligned. Match its residue so the rebased native index remains exact.
        const uint64_t aligned = offset + ((nativeBase - offset) & (MatrixBytes - 1));
        if (aligned + TailMatrixBufferBytes > 0x4000000) {
            throw std::runtime_error("Dynamic-tail matrices exceed the graphics address range.");
        }
        buffer.address = static_cast<uint32_t>(aligned) | 0x80000000;
    }
    const uint32_t baseIndex = (buffer.address - nativeBase) / MatrixBytes;
    const int32_t savedCount = MEM_W(ModelMatrixCountOffset, vram(display));
    const uint32_t startIndex = baseIndex + buffer.used;
    const uint32_t commandsBegin = static_cast<uint32_t>(MEM_W(0, OpaqueDisplayListHead));
    MEM_W(ModelMatrixCountOffset, vram(display)) = startIndex;
    activeTailPass = pass;
    try {
        if (pass == 0) func_802DE950_6F0000(rdram, ctx);
        else func_802E072C_6F1DDC(rdram, ctx);
    } catch (...) {
        MEM_W(ModelMatrixCountOffset, vram(display)) = savedCount;
        activeTailPass = -1;
        throw;
    }
    const uint32_t endIndex = static_cast<uint32_t>(MEM_W(ModelMatrixCountOffset, vram(display)));
    MEM_W(ModelMatrixCountOffset, vram(display)) = savedCount;
    activeTailPass = -1;
    if (endIndex < startIndex || endIndex - startIndex > maximumWrites) {
        throw std::runtime_error("Unexpected dynamic-tail matrix usage.");
    }
    // Native gSPMatrix uses OS_K0_TO_PHYSICAL. Above 16 MiB that stripped
    // address looks like a segment-1 pointer to the RSP, rather than extended
    // RDRAM. Restore RT64's direct-address flag only on this pass's matrices.
    const uint32_t commandsEnd = static_cast<uint32_t>(MEM_W(0, OpaqueDisplayListHead));
    if (commandsEnd < commandsBegin || (commandsEnd - commandsBegin) % 8 != 0) {
        throw std::runtime_error("Invalid dynamic-tail display-list range.");
    }
    const uint32_t physicalBegin = (buffer.address & 0x1FFFFFFF) + buffer.used * MatrixBytes;
    const uint32_t physicalEnd = physicalBegin + (endIndex - startIndex) * MatrixBytes;
    for (uint32_t command = commandsBegin; command < commandsEnd; command += 8) {
        const uint32_t word0 = static_cast<uint32_t>(MEM_W(0, vram(command)));
        const uint32_t pointer = static_cast<uint32_t>(MEM_W(4, vram(command)));
        if ((word0 >> 24) == 0x01 && pointer >= physicalBegin && pointer < physicalEnd) {
            MEM_W(4, vram(command)) = pointer | 0x80000000;
        }
    }
    buffer.used = endIndex - baseIndex;
    if (buffer.used > tailHighWater) {
        tailHighWater = buffer.used;
        static std::ofstream log("display-list-capacity.log", std::ios::app);
        log << "dynamic_tail_matrices=" << tailHighWater << "; capacity=" << TailMatrixCapacity
            << "; native_model_matrices=" << savedCount << std::endl;
    }
    return 1;
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
    const uint32_t opaqueBuffer = opaqueBuffers[frame_index(frame)];
    const uint32_t opaqueEnd = static_cast<uint32_t>(MEM_W(0, OpaqueDisplayListHead));
    const uint32_t opaqueBytes = opaqueEnd - opaqueBuffer;
    if (opaqueBuffer == 0 || opaqueBytes > BufferBytes || opaqueBytes % 8 != 0) {
        throw std::runtime_error("Opaque display-list buffer capacity exceeded.");
    }
    if (opaqueBytes / 8 > opaqueHighWater) {
        opaqueHighWater = opaqueBytes / 8;
        if (opaqueHighWater > 1200) {
            static std::ofstream log("display-list-capacity.log", std::ios::app);
            log << "opaque_display_list_commands=" << opaqueHighWater << "; capacity=" << CommandCapacity << std::endl;
        }
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

namespace {
struct ReflectionAnimalRange { uint32_t begin[2], end[2]; int32_t coordinate; uint8_t opacity; };
std::vector<ReflectionAnimalRange> reflectionRanges;
ReflectionAnimalRange pendingReflection{};
std::array<std::array<uint32_t, 2>, 2> reflectionCopies{};
constexpr int32_t XluDisplayListHead = static_cast<int32_t>(0x801D9E8C);
constexpr int32_t ReflectionVisibilityMask = static_cast<int32_t>(0x803F28DC);
}
void sssv_reset_reflection_ranges() { reflectionRanges.clear(); }

// Hooks surround just the native animal draw dispatch. Normal lists are never modified.
extern "C" void sssv_reflection_animal_range(uint8_t* rdram, recomp_context*, int phase) {
    const uint16_t mask = MEM_HU(0, ReflectionVisibilityMask);
    if (!(mask & 3)) return;
    if (phase == 0) {
        pendingReflection.begin[0] = static_cast<uint32_t>(MEM_W(0, OpaqueDisplayListHead));
        pendingReflection.begin[1] = static_cast<uint32_t>(MEM_W(0, XluDisplayListHead));
        const gpr animal = MEM_W(0, static_cast<int32_t>(0x803D5530));
        pendingReflection.coordinate = MEM_H((mask & 1) ? 12 : 4, animal);
    } else {
        pendingReflection.end[0] = static_cast<uint32_t>(MEM_W(0, OpaqueDisplayListHead));
        pendingReflection.end[1] = static_cast<uint32_t>(MEM_W(0, XluDisplayListHead));
        const int32_t plane = (mask & 0xFFC) * 2;
        pendingReflection.opacity = sssv::reflection_opacity(std::abs(pendingReflection.coordinate - plane));
        reflectionRanges.push_back(pendingReflection);
    }
}

extern "C" void recomp_get_near_reflection_dl(uint8_t* rdram, recomp_context* ctx) {
    const auto frame = static_cast<uint32_t>(MEM_W(0, DisplayListContextPointer));
    const auto index = frame_index(frame - DisplayListOffset);
    const unsigned list = ctx->r5 ? 1 : 0;
    const uint32_t original = static_cast<uint32_t>(ctx->r4);
    uint32_t source = list ? original : opaqueBuffers[index];
    const uint32_t end = static_cast<uint32_t>(MEM_W(0, list ? XluDisplayListHead : OpaqueDisplayListHead));
    const uint32_t capacity = list ? 1000 : CommandCapacity;
    if (source > end || (end-source) % 8 || (end-source)/8 > capacity)
        throw std::runtime_error("Invalid reflection display-list bounds.");
    auto& copy = reflectionCopies[index][list];
    if (!copy) {
        auto* allocation = static_cast<uint8_t*>(recomp::alloc(rdram, (capacity+4096)*8));
        if (!allocation) throw std::runtime_error("Could not allocate reflection list.");
        copy = static_cast<uint32_t>(allocation-rdram) | 0x80000000u;
    }
    uint32_t output = copy + (list ? 0 : 40);
    auto emit = [&](uint32_t w0, uint32_t w1) {
        if (output+8 > copy+(capacity+4096)*8) throw std::runtime_error("Reflection list capacity exceeded.");
        MEM_W(0,vram(output))=w0; MEM_W(4,vram(output))=w1; output+=8;
    };
    for (uint32_t at=source; at<end; at+=8) {
        const ReflectionAnimalRange* animal = nullptr;
        for (const auto& range : reflectionRanges)
            if (at>=range.begin[list] && at<range.end[list]) { animal=&range; break; }
        if (animal && at==animal->begin[list]) {
            // Private reflection marker: native animal materials do not use blend colour.
            // Preserves all native material combiners while fading the complete animal.
            emit(0xF9000000u, 0x53565200u | animal->opacity);
        }
        const uint32_t w0=static_cast<uint32_t>(MEM_W(0,vram(at)));
        const uint32_t w1=static_cast<uint32_t>(MEM_W(4,vram(at)));
        // Separately queued tails use absolute world-space matrices in their
        // own arena. Apply the same fade so detached tails cannot reflect alone.
        if (!animal && (w0>>24)==0x01) {
            const auto& tails=tailBuffers[index];
            const uint32_t address=w1 & 0x03FFFFFFu;
            const uint32_t tailStart=tails.address & 0x03FFFFFFu;
            if(tails.address && address>=tailStart && address+64<=tailStart+tails.used*64) {
                const uint16_t mask=MEM_HU(0,ReflectionVisibilityMask);
                const int position=MEM_H((mask&1)?28:24,vram(address|0x80000000u));
                emit(0xF9000000u,0x53565200u | sssv::reflection_opacity(std::abs(position-(mask&0xFFC)*2)));
            }
        }
        emit(w0,w1);
        if(!animal && (w0>>24)==0xBD) emit(0xF9000000u,0);
        if (animal && at+8==animal->end[list]) emit(0xF9000000u,0);
    }
    emit(0xF9000000u,0);
    emit(0xB8000000u,0);
    if (!list) {
        // Preserve the expanded-address wrapper around the new opaque copy.
        for(unsigned offset=0;offset<40;offset+=4)
            MEM_W(offset,vram(copy))=MEM_W(offset,vram(original));
        MEM_W(20,vram(copy))=vram(copy+40);
    }
    ctx->r2=vram(copy);
}
