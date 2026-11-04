#pragma once
#include <cstdint>
#include <deque>
namespace sssv {
class FastForwardState {
    bool previous = false, latched = false, last_toggle = false;
    bool hold_blocked = false, burst_blocked = false;
    uint64_t held_since = 0, last_press = 0;
    std::deque<uint64_t> presses;
public:
    uint32_t update(bool pressed, bool toggle, uint32_t rate, bool enabled, uint64_t now_ms) {
        if (!enabled || rate <= 1 || toggle != last_toggle) latched = false;
        const bool rising = pressed && !previous;
        if (rising) held_since = now_ms;
        if (!pressed) hold_blocked = false;
        if (burst_blocked && now_ms - last_press > 2000) burst_blocked = false;
        while (!presses.empty() && now_ms - presses.front() > 2000) presses.pop_front();
        if (toggle && rising) {
            last_press = now_ms;
            presses.push_back(now_ms);
            if (presses.size() > 50) { burst_blocked = true; presses.clear(); }
        }
        if (enabled && rate > 1 && toggle && rising && !hold_blocked && !burst_blocked) latched = !latched;
        if (toggle && pressed && now_ms - held_since > 2000) hold_blocked = true;
        if (hold_blocked || burst_blocked) latched = false;
        previous = pressed;
        last_toggle = toggle;
        return enabled && rate > 1 && (toggle ? latched : pressed) ? rate : 1;
    }
};
}
