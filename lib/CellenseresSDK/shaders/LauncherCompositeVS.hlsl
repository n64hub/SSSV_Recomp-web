// The page's composite of the launcher's 3D logo (launcher_model3d.cpp, composite_offscreen_logo()): a triangle that
// covers the whole target, no vertex buffer.

struct VSOutput {
    float4 position : SV_POSITION;
};

VSOutput VSMain(uint vertex_id : SV_VertexID) {
    VSOutput output;
    float2 uv = float2((vertex_id << 1) & 2, vertex_id & 2);
    output.position = float4(uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
}
