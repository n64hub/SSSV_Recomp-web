// Standalone regression test for the real host-side tail arena. Link this file
// with src/game/display_list_buffers.cpp and the normal recomp include paths.
#include <cassert>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "recomp.h"

extern "C" int sssv_render_dynamic_tail_pass(uint8_t*, recomp_context*, int);
extern "C" void sssv_begin_main_display_list(uint8_t*, recomp_context*);
extern "C" void sssv_submit_main_display_list(uint8_t*, recomp_context*);

namespace {
constexpr uint32_t frames[2] = {0x80162658, 0x8019E248};
constexpr uint32_t displayOffset = 0x4E0;
constexpr uint32_t poolOffset = 0x33590;
constexpr uint32_t countOffset = 0x38918;
constexpr int32_t displayPointer = static_cast<int32_t>(0x80204278);
constexpr int32_t tailCountAddress = static_cast<int32_t>(0x803E1B02);
constexpr int32_t opaqueHead = static_cast<int32_t>(0x801D9E88);
constexpr uint32_t commandsBase = 0x80400000;
uint32_t allocationOffset = 0x1000000;
std::vector<uint32_t> writes;
bool failNative = false;
int nativeCalls = 0;

gpr addr(uint32_t value) { return static_cast<int64_t>(static_cast<int32_t>(value)); }

void native_pass(uint8_t* rdram, recomp_context* ctx, int pass) {
    if (sssv_render_dynamic_tail_pass(rdram, ctx, pass)) return;
    ++nativeCalls;
    const uint32_t display = MEM_W(0, displayPointer);
    const int count = MEM_H(0, tailCountAddress) * (pass == 0 ? 1 : 6);
    for (int i = 0; i < count; ++i) {
        const uint32_t index = MEM_W(countOffset, addr(display));
        const uint32_t matrix = display + poolOffset + (index << 6);
        assert((matrix & 7) == 0);
        writes.push_back(matrix);
        std::memset(rdram + (matrix & 0x1FFFFFFF), 0x55, 64);
        const uint32_t command = MEM_W(0, opaqueHead);
        MEM_W(0, addr(command)) = 0x01040040; // Native F3DEX gSPMatrix.
        MEM_W(4, addr(command)) = matrix & 0x1FFFFFFF;
        MEM_W(8, addr(command)) = 0x06000000; // Same pointer, different opcode.
        MEM_W(12, addr(command)) = matrix & 0x1FFFFFFF;
        MEM_W(16, addr(command)) = 0x01040040; // Native matrix outside our arena.
        MEM_W(20, addr(command)) = 0x00100000;
        MEM_W(0, opaqueHead) = command + 24;
        MEM_W(countOffset, addr(display)) = index + 1;
        if (failNative) throw std::runtime_error("simulated render failure");
    }
}

template<class F> void throws(F fn) {
    bool caught = false;
    try { fn(); } catch (const std::runtime_error&) { caught = true; }
    assert(caught);
}
}

namespace recomp {
void* alloc(uint8_t* rdram, size_t size) {
    const uint32_t start = allocationOffset;
    allocationOffset += static_cast<uint32_t>((size + 15) & ~size_t(15));
    assert(allocationOffset < 0x4000000);
    return rdram + start;
}
}

extern "C" void func_802DE950_6F0000(uint8_t* rdram, recomp_context* ctx) { native_pass(rdram, ctx, 0); }
extern "C" void func_802E072C_6F1DDC(uint8_t* rdram, recomp_context* ctx) { native_pass(rdram, ctx, 1); }

int main() {
    std::vector<uint8_t> memory(0x4000000);
    uint8_t* rdram = memory.data();
    recomp_context ctx{};
    auto begin = [&](int frame) {
        ctx.r4 = addr(frames[frame]);
        sssv_begin_main_display_list(rdram, &ctx);
        const uint32_t display = frames[frame] + displayOffset;
        MEM_W(0, displayPointer) = display;
        MEM_W(countOffset, addr(display)) = 237;
        MEM_W(0, opaqueHead) = commandsBase;
        std::memset(rdram + (display & 0x1FFFFFFF) + poolOffset, 0xA5, 250 * 64 + 4 * 64);
        writes.clear();
    };
    auto preserved = [&](int frame) {
        const uint32_t display = frames[frame] + displayOffset;
        assert(MEM_W(countOffset, addr(display)) == 237);
        for (int i = 0; i < 254 * 64; ++i)
            assert(rdram[(display & 0x1FFFFFFF) + poolOffset + i] == 0xA5);
    };
    auto packets = [&] {
        for (size_t i = 0; i < writes.size(); ++i) {
            const uint32_t command = commandsBase + static_cast<uint32_t>(i) * 24;
            assert(static_cast<uint32_t>(MEM_W(4, addr(command))) == writes[i]);
            assert(static_cast<uint32_t>(MEM_W(12, addr(command))) == (writes[i] & 0x1FFFFFFF));
            assert(static_cast<uint32_t>(MEM_W(20, addr(command))) == 0x00100000);
        }
    };

    begin(0);
    MEM_H(0, tailCountAddress) = 120;
    const auto savedContext = ctx;
    func_802DE950_6F0000(rdram, &ctx);
    func_802E072C_6F1DDC(rdram, &ctx);
    assert(writes.size() == 840 && nativeCalls == 2);
    assert(std::memcmp(&ctx, &savedContext, sizeof(ctx)) == 0);
    const uint32_t firstBuffer = writes.front();
    for (size_t i = 1; i < writes.size(); ++i) assert(writes[i] == writes[i - 1] + 64);
    preserved(0);
    packets();

    // A repeated model pass appends, and the next impossible worst-case reserve
    // fails before the native renderer can write even one matrix.
    func_802E072C_6F1DDC(rdram, &ctx);
    assert(writes.size() == 1560);
    throws([&] { func_802E072C_6F1DDC(rdram, &ctx); });
    assert(writes.size() == 1560 && nativeCalls == 3);
    preserved(0);
    packets();

    begin(1);
    func_802E072C_6F1DDC(rdram, &ctx);
    const uint32_t secondBuffer = writes.front();
    assert(secondBuffer != firstBuffer && writes.size() == 720);
    preserved(1);
    packets();
    begin(0);
    func_802DE950_6F0000(rdram, &ctx);
    assert(writes.front() == firstBuffer && writes.size() == 120);
    preserved(0);
    packets();

    MEM_H(0, tailCountAddress) = 121;
    throws([&] { func_802E072C_6F1DDC(rdram, &ctx); });
    assert(writes.size() == 120);
    MEM_H(0, tailCountAddress) = -1;
    throws([&] { func_802DE950_6F0000(rdram, &ctx); });
    assert(writes.size() == 120);
    MEM_H(0, tailCountAddress) = 1;
    failNative = true;
    throws([&] { func_802E072C_6F1DDC(rdram, &ctx); });
    preserved(0);
    failNative = false;
    begin(0);
    func_802E072C_6F1DDC(rdram, &ctx);
    assert(writes.size() == 6 && writes.front() == firstBuffer);
    preserved(0);
    packets();

    // A 1439-command opaque list used to overwrite the neighboring translucent
    // list. Exercise the actual frame setup/submit path and embedded branch.
    uint32_t opaque[2]{};
    for (int frame = 0; frame < 2; ++frame) {
        ctx.r4 = addr(frames[frame]);
        sssv_begin_main_display_list(rdram, &ctx);
        opaque[frame] = MEM_W(0, opaqueHead);
        const uint32_t display = frames[frame] + displayOffset;
        assert(static_cast<uint32_t>(MEM_W(0, addr(display + 0x9600))) == 0x00525464);
        assert(static_cast<uint32_t>(MEM_W(4, addr(display + 0x9600))) == 0x10000064);
        assert(static_cast<uint32_t>(MEM_W(8, addr(display + 0x9600))) == 0x6400002C);
        assert(static_cast<uint32_t>(MEM_W(12, addr(display + 0x9600))) == 1);
        assert(static_cast<uint32_t>(MEM_W(16, addr(display + 0x9600))) == 0x06000000);
        assert(static_cast<uint32_t>(MEM_W(20, addr(display + 0x9600))) == opaque[frame]);
        assert(static_cast<uint32_t>(MEM_W(24, addr(display + 0x9600))) == 0x6400002C);
        assert(static_cast<uint32_t>(MEM_W(28, addr(display + 0x9600))) == 0);
        assert(static_cast<uint32_t>(MEM_W(32, addr(display + 0x9600))) == 0xB8000000);
        assert(static_cast<uint32_t>(MEM_W(36, addr(display + 0x9600))) == 0);
        assert((opaque[frame] & 0x80000000) != 0);
        assert(static_cast<uint32_t>(MEM_W(0, addr(opaque[frame]))) == 0xB8000000);
        // Interpret the real wrapper's call/return flow. The external list
        // requires direct addressing, but the following energy sprite requires
        // the legacy segment-1 interpretation of its 0x81040CB0 texture alias.
        std::vector<uint32_t> returns;
        uint32_t pc = display + 0x9600;
        bool extended = false;
        bool enteredExternal = false;
        for (int step = 0; step < 8; ++step) {
            if (pc == opaque[frame]) {
                assert(extended);
                enteredExternal = true;
            }
            const uint32_t op = MEM_W(0, addr(pc));
            const uint32_t value = MEM_W(4, addr(pc));
            if (op == 0x6400002C) extended = value != 0;
            else if ((op >> 24) == 6) {
                assert(extended && (value & 0x80000000));
                if ((op & 0x10000) == 0) returns.push_back(pc + 8);
                pc = value;
                continue;
            } else if (op == 0xB8000000) {
                if (returns.empty()) break;
                pc = returns.back();
                returns.pop_back();
                continue;
            }
            pc += 8;
        }
        assert(enteredExternal && returns.empty() && !extended);
        const uint32_t energyAlias = 0x81040CB0;
        const uint32_t segment1 = 0x00300000;
        const uint32_t resolvedEnergy = extended ? energyAlias - 0x80000000 : segment1 + (energyAlias & 0xFFFFFF);
        assert(resolvedEnergy == 0x00340CB0);
        std::memset(rdram + (display & 0x1FFFFFFF) + 0xBB80, 0xA7, 8000);
        std::memset(rdram + (opaque[frame] & 0x1FFFFFFF), 0x55, 1439 * 8);
        MEM_W(0, opaqueHead) = opaque[frame] + 1439 * 8;
        const uint32_t main = MEM_W(0, static_cast<int32_t>(0x801D9E7C));
        MEM_W(0, addr(main)) = 0x06000000;
        MEM_W(4, addr(main)) = display + 0x9600; // Existing callers need no change.
        MEM_W(0, static_cast<int32_t>(0x801D9E7C)) = main + 8;
        ctx.r5 = addr(display);
        sssv_submit_main_display_list(rdram, &ctx);
        assert(static_cast<uint32_t>(ctx.r5) == main && ctx.r6 == 8);
        assert(static_cast<uint32_t>(MEM_W(4, addr(main))) == display + 0x9600);
        for (int i = 0; i < 8000; ++i)
            assert(rdram[(display & 0x1FFFFFFF) + 0xBB80 + i] == 0xA7);
        MEM_W(0, opaqueHead) = opaque[frame] + 16385 * 8;
        ctx.r5 = addr(display);
        throws([&] { sssv_submit_main_display_list(rdram, &ctx); });
    }
    assert(opaque[0] != opaque[1]);
    ctx.r4 = addr(frames[0]);
    sssv_begin_main_display_list(rdram, &ctx);
    assert(static_cast<uint32_t>(MEM_W(0, opaqueHead)) == opaque[0]);
    std::cout << "Tail matrix arena and opaque display-list relocation: frame isolation/reuse, bounds, preserved native state, extended-address packets, scoped address mode, legacy energy texture addressing and exception recovery passed.\n";
}
