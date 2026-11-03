#include "../../lib/rt64/src/gbi/sssv_terminal_feedback.h"
#include "sssv_mirror_input.h"
#include "gbi/sssv_mirror.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <vector>

int main() {
    for (unsigned nativeRate : {30u, 60u}) {
        for (unsigned rate : {60u, 120u, 240u, 360u}) {
            for (float alpha : {75.0f / 255.0f, 100.0f / 255.0f}) {
                const float corrected = SSSVTerminalFeedback::fadeAlpha(alpha, nativeRate, rate);
                const float retained = std::pow(1.0f - corrected, float(rate) / nativeRate);
                assert(std::abs(retained - (1.0f - alpha)) < 0.00001f);
            }
        }
    }
    assert(SSSVTerminalFeedback::fadeAlpha(1.0f, 30, 240) == 1.0f);
    assert(SSSVTerminalFeedback::fadeAlpha(0.0f, 30, 240) == 0.0f);
    assert(SSSVTerminalFeedback::fadeAlpha(0.4f, 0, 240) == 0.4f);

    using namespace sssv::mirror;
    for (unsigned bits = 0; bits <= 0xffff; ++bits) {
        uint16_t buttons = uint16_t(bits);
        float x = 0.75f;
        reflect_input(buttons, x);
        assert(x == -0.75f);
        assert((buttons & 0xfcfc) == (bits & 0xfcfc));
        assert(bool(buttons & 0x200) == bool(bits & 0x100));
        assert(bool(buttons & 2) == bool(bits & 1));
        reflect_input(buttons, x);
        assert(buttons == bits && x == 0.75f);
    }
    std::vector<uint8_t> ram(8*1024*1024);
    const auto put16 = [&](uint32_t a, uint16_t v) { std::memcpy(ram.data()+((a&0x7fffff)^2),&v,2); };
    const auto put8 = [&](uint32_t a,uint8_t v) { ram[(a&0x7fffff)^3]=v; };
    assert(!gameplay_input(nullptr) && !gameplay_input(ram.data()));
    put16(0x803f2d38,26); assert(gameplay_input(ram.data()));
    put16(0x803f6680,1); assert(!gameplay_input(ram.data()));
    put16(0x803f6680,0);put8(0x803f2d10,2);assert(!gameplay_input(ram.data()));
    put8(0x803f2d10,0);put16(0x803e1bc0,1);assert(!gameplay_input(ram.data()));
    put16(0x803e1bc0,0);put8(0x803f2aa2,3);assert(!gameplay_input(ram.data()));
    put8(0x803f2aa2,0);assert(gameplay_input(ram.data()));
    assert(RT64::SSSVMirror::worldGroup(0x02000000));
    assert(RT64::SSSVMirror::worldProjection(0x1000));
    assert(RT64::SSSVMirror::worldProjection(0x1020));
    assert(RT64::SSSVMirror::worldProjection(0x1021));
    assert(!RT64::SSSVMirror::worldProjection(0x1001));
    assert(!RT64::SSSVMirror::worldGroup(0x10000001));
    assert(!RT64::SSSVMirror::worldGroup(0x12000001));
    const auto path=std::filesystem::temp_directory_path()/"sssv-mirror-rules-unit.tex";
    {std::ofstream f(path);f<<"# comment\n32 32 0x1234 0x1234 # sign\ninvalid\n0 1 0x42 0x42\n32 32 0xbad 0x111\n";}
    RT64::SSSVMirror::loadRules(path);
    assert(RT64::SSSVMirror::readable(0x1234));
    assert(!RT64::SSSVMirror::readable(0x42));
    assert(!RT64::SSSVMirror::readable(0xbad));
    std::filesystem::remove(path);RT64::SSSVMirror::loadRules(path);
    assert(RT64::SSSVMirror::readableTextures.empty());
}
