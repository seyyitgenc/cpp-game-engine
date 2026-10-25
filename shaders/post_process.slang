// Post-process pass: exposure, then tonemapping, from linear HDR into the
// scene image the Viewport shows. sRGB encoding happens here when the target
// format does not do it (the scene image is UNORM).

import common.fullscreen;

// Set 1: the lighting pass output (PostProcessPass::CreateDescriptorSet).
// Set 0 is the per-frame global set, bound like in every pass but unused here.
[[vk::binding(0, 1)]] Texture2D<float4> hdrColor;

// C++: PostProcessPushConstants in Renderer/ShaderTypes.hpp.
struct PostData {
    float exposure;
    uint encodeSrgb; // 1 if the target format does not encode by itself
};

[[vk::push_constant]] ConstantBuffer<PostData> post;

// Khronos PBR Neutral (Khronos 3D Commerce, 2024). Made for glTF PBR
// content: colors below ~0.8 come out practically unchanged, so a base color
// under plain light looks like its base color, and only highlights are
// compressed, desaturating towards white as they approach it.
float3 PbrNeutralToneMap(float3 color) {
    const float startCompression = 0.8 - 0.04;
    const float desaturation = 0.15;

    float x = min(color.r, min(color.g, color.b));
    float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;

    float peak = max(color.r, max(color.g, color.b));
    if (peak < startCompression) return color;

    const float d = 1.0 - startCompression;
    float newPeak = 1.0 - d * d / (peak + d - startCompression);
    color *= newPeak / peak;

    float g = 1.0 - 1.0 / (desaturation * (peak - newPeak) + 1.0);
    return lerp(color, float3(newPeak), g);
}

// The sRGB transfer function, for when the hardware does not apply it.
float3 LinearToSrgb(float3 color) {
    float3 low = color * 12.92;
    float3 high = 1.055 * pow(color, 1.0 / 2.4) - 0.055;
    return select(color <= 0.0031308, low, high);
}

struct VSOutput {
    float4 position : SV_Position;
};

[shader("vertex")]
VSOutput vertMain(uint vertexId : SV_VulkanVertexID) {
    VSOutput output;
    output.position = FullscreenTrianglePosition(vertexId);
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target {
    float3 color = hdrColor.Load(int3(int2(input.position.xy), 0)).rgb;

    color = PbrNeutralToneMap(color * post.exposure);

    if (post.encodeSrgb != 0) {
        color = LinearToSrgb(saturate(color));
    }
    return float4(color, 1.0);
}
