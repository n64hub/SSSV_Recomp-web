#ifndef SSSV_BILLBOARD_IDENTITY_H
#define SSSV_BILLBOARD_IDENTITY_H
/* Only category zero selects animated collectible frames. Plants use one image. */
static inline int sssv_billboard_animation_signature(int category, int frame) {
    return category == 0 ? frame : 0;
}
/* A newly visible static sprite must never steal a neighbour's transform ID. */
static inline int sssv_billboard_position_matches(int stationary, int dx, int dz, int dy) {
    return !stationary || (dx == 0 && dz == 0 && dy == 0);
}
#endif
