// Lighting pass: shades the G-buffer with one fullscreen triangle,
// Cook-Torrance per light, into linear HDR.

import common.frame;
import common.gbuffer;
import common.brdf;
import common.fullscreen;

// Set 1: the G-buffer (LightingPass::CreateDescriptorSet). Read with Load(),
// one texel per pixel, so there is no sampler and no filtering.
[[vk::binding(0, 1)]] Texture2D<float4> gBaseColor;
[[vk::binding(1, 1)]] Texture2D<float2> gNormal;
[[vk::binding(2, 1)]] Texture2D<float4> gMaterial;
[[vk::binding(3, 1)]] Texture2D<float4> gEmissive;
[[vk::binding(4, 1)]] Texture2D<float>  gDepth;

struct VSOutput {
    float4 position : SV_Position;
};

[shader("vertex")]
VSOutput vertMain(uint vertexId : SV_VulkanVertexID) {
    VSOutput output;
    output.position = FullscreenTrianglePosition(vertexId);
    return output;
}

// Undoes the projection for this pixel. pixel is SV_Position.xy, i.e. pixel
// centers. Depth is already in Vulkan's 0..1 range, so it goes in as clip z
// directly; the Y flip baked into the projection is undone by its inverse.
float3 ReconstructWorldPosition(float2 pixel, float depth) {
    uint width, height;
    gDepth.GetDimensions(width, height);

    float2 ndc = pixel / float2(width, height) * 2.0 - 1.0;
    float4 world = mul(camera.inverseViewProjection, float4(ndc, depth, 1.0));
    return world.xyz / world.w;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target {
    int3 texel = int3(int2(input.position.xy), 0);

    // Depth was cleared to the far plane, so 1 means no geometry here.
    float depth = gDepth.Load(texel);
    if (depth >= 1.0) return float4(0.0, 0.0, 0.0, 1.0);

    float4 orm = gMaterial.Load(texel);

    Surface s;
    s.position = ReconstructWorldPosition(input.position.xy, depth);
    s.normal = DecodeNormal(gNormal.Load(texel));
    s.view = normalize(camera.position.xyz - s.position);
    s.baseColor = gBaseColor.Load(texel).rgb;
    s.metallic = orm.b;
    s.roughness = orm.g;

    float3 color = float3(0.0);

    uint lightCount = min(lighting.lightCount, MAX_LIGHTS);
    for (uint i = 0; i < lightCount; ++i) {
        color += EvaluateLight(lighting.lights[i], s);
    }

    // Placeholder for image based lighting: a flat ambient term. Ambient
    // occlusion applies here only; direct light has its own (shadow) term.
    float occlusion = orm.r;
    color += lighting.ambientColor * s.baseColor * occlusion;

    color += gEmissive.Load(texel).rgb;

    return float4(color, 1.0);
}
