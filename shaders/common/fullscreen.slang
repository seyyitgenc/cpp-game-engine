// For passes that cover the whole screen and read their inputs per pixel.
//
// A single triangle with corners (-1,-1), (3,-1) and (-1,3) covers the whole
// screen, with no vertex buffer and no diagonal seam down the middle like a
// two triangle quad has. Draw 3 vertices, and feed the entry point's
// SV_VulkanVertexID in.
//
// SV_VulkanVertexID, not SV_VertexID: Slang gives SV_VertexID D3D's meaning,
// gl_VertexIndex - gl_BaseVertex, and gl_BaseVertex needs the
// shaderDrawParameters device feature.
float4 FullscreenTrianglePosition(uint vertexId) {
    float2 uv = float2((vertexId << 1) & 2, vertexId & 2);
    return float4(uv * 2.0 - 1.0, 0.0, 1.0);
}
