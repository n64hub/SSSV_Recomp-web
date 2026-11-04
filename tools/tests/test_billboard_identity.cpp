#include <cassert>
#include "../../patches/sssv_billboard_identity.h"
int main() {
 for(int category=1;category<32;++category)
  for(int phase=0;phase<100;++phase) assert(sssv_billboard_animation_signature(category,phase)==0);
 assert(sssv_billboard_animation_signature(0,3)==3);
 assert(sssv_billboard_position_matches(1,0,0,0));
 assert(!sssv_billboard_position_matches(1,1,0,0));
 assert(!sssv_billboard_position_matches(1,0,-1,0));
 assert(!sssv_billboard_position_matches(1,0,0,1));
 assert(sssv_billboard_position_matches(0,64,64,64));
}
