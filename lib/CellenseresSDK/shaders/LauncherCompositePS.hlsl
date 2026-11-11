// The page's composite of the launcher's 3D logo (launcher_model3d.cpp, composite_offscreen_logo()): copies the
// pixels the logo drew in its own target (alpha 1, LauncherModelPS) onto the launcher's, and leaves the rest, the
// target's clear (alpha 0), as the launcher drew it.

Texture2D<float4> gSource : register(t0);

float4 PSMain(float4 position : SV_POSITION) : SV_TARGET {
    float4 color = gSource.Load(int3(int2(position.xy), 0));
    if (color.a <= 0.0f) {
        discard;
    }
    return color;
}
