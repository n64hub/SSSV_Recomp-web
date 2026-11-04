#include "sssv_fast_forward.h"
#include "ultramodern/speed_clock.hpp"
#include <cassert>
#include <iostream>
int main() {
    ultramodern::SpeedClock clock;
    assert(clock.now(100) == 100);
    clock.set(100,4); assert(clock.now(200)==500);
    clock.set(200,1); assert(clock.now(300)==600);
    for (unsigned speed : {2u,4u,8u,16u}) {
        auto before=clock.now(1000); clock.set(1000,speed);
        assert(clock.now(1000)==before); assert(clock.now(1100)==before+100*speed);
        clock.set(1100,1); assert(clock.now(1200)==before+100*speed+100);
    }
    sssv::FastForwardState held;
    assert(held.update(false,false,4,true,0)==1);
    assert(held.update(true,false,4,true,1)==4);
    assert(held.update(true,false,4,true,4000)==4); // Held has no timeout.
    assert(held.update(false,false,4,true,4001)==1);
    assert(held.update(true,false,1,true,4002)==1); // Disabled.
    assert(held.update(true,false,8,false,4003)==1); // Focus/menu suppression.
    sssv::FastForwardState toggle;
    assert(toggle.update(true,true,4,true,0)==4);
    assert(toggle.update(false,true,4,true,10)==4);
    assert(toggle.update(true,true,4,true,20)==1);
    toggle.update(false,true,4,true,30);
    assert(toggle.update(true,true,4,true,40)==4);
    assert(toggle.update(true,true,4,true,2040)==4);
    assert(toggle.update(true,true,4,true,2041)==1);
    assert(toggle.update(true,true,4,true,3000)==1);
    toggle.update(false,true,4,true,3001);
    assert(toggle.update(true,true,4,true,3002)==4);
    assert(toggle.update(false,true,4,false,3003)==1);
    assert(toggle.update(false,true,4,true,3004)==1); // Doesn't relatch after focus loss.
    sssv::FastForwardState burst;
    for (unsigned i=0;i<50;i++) {
        burst.update(true,true,4,true,i*20); burst.update(false,true,4,true,i*20+1);
    }
    assert(burst.update(true,true,4,true,1000)==1); // 51st press is a failsafe.
    burst.update(false,true,4,true,1001);
    assert(burst.update(true,true,4,true,1100)==1);
    burst.update(false,true,4,true,3102);
    assert(burst.update(true,true,4,true,3103)==4);
    std::cout << "Fast-forward clock, all rates, held/toggle and failsafes passed.\n";
}
