#include "sssv_reflection_fade.h"
#include <cassert>
int main() {
    assert(sssv::reflection_opacity(0)==255);
    assert(sssv::reflection_opacity(256)==255);
    assert(sssv::reflection_opacity(288)==128);
    assert(sssv::reflection_opacity(320)==0);
    assert(sssv::reflection_opacity(4096)==0);
    for(int d=1;d<4096;++d) assert(sssv::reflection_opacity(d)<=sssv::reflection_opacity(d-1));
}
