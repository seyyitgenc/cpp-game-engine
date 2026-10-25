// Post-process pass: exposure, then tonemapping, from linear HDR into the
// scene image the Viewport shows, then the editor's outlines. sRGB encoding
// happens here when the target format does not do it (the scene image is
// UNORM).

import common.fullscreen;

// Set 1 (PostProcessPass::CreateDescriptorSet): the lighting pass output and
// the G-buffer's entity ids. Set 0 is the per-frame global set, bound like in
// every pass but unused here.
[[vk::binding(0, 1)]] Texture2D<float4> hdrColor;
[[vk::binding(1, 1)]] Texture2D<uint> entityIds;

// C++: PostProcessPushConstants in Renderer/ShaderTypes.hpp.
struct PostData {
    float exposure;
    uint encodeSrgb; // 1 if the target format does not encode by itself
    uint selectedId; // entity ids to outline, 0 for none
    uint hoveredId;
};

[[vk::push_constant]] ConstantBuffer<PostData> post;

// Applied after encoding, so these are the colors that show.
static const int OUTLINE_RADIUS = 2; // pixels, outside the silhouette
static const float3 SELECTED_OUTLINE = float3(1.0, 0.6, 0.1);
static const float4 HOVERED_OUTLINE = float4(1.0, 1.0, 1.0, 0.5); // a: opacity

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

float3 ApplyOutlines(float3 color, int2 pixel) {
    // The same for every pixel of the draw, so this costs nothing when
    // nothing is selected or hovered.
    if (post.selectedId == 0 && post.hoveredId == 0) return color;

    uint width, height;
    entityIds.GetDimensions(width, height);
    const int2 last = int2(width, height) - 1;

    bool nearSelected = false;
    bool nearHovered = false;
    for (int y = -OUTLINE_RADIUS; y <= OUTLINE_RADIUS; ++y) {
        for (int x = -OUTLINE_RADIUS; x <= OUTLINE_RADIUS; ++x) {
            // Rounds the outline's corners.
            if (x * x + y * y > OUTLINE_RADIUS * OUTLINE_RADIUS + 1) continue;
            // Load() outside the image is undefined without robustness features.
            uint id = entityIds.Load(int3(clamp(pixel + int2(x, y), int2(0), last), 0));
            nearSelected = nearSelected || id == post.selectedId;
            nearHovered = nearHovered || id == post.hoveredId;
        }
    }

    // Only outside the silhouette, so an outline never covers its entity.
    uint center = entityIds.Load(int3(pixel, 0));
    if (post.selectedId != 0 && nearSelected && center != post.selectedId) return SELECTED_OUTLINE;
    if (post.hoveredId != 0 && nearHovered && center != post.hoveredId) return lerp(color, HOVERED_OUTLINE.rgb, HOVERED_OUTLINE.a);
    return color;
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
    const int2 pixel = int2(input.position.xy);
    float3 color = hdrColor.Load(int3(pixel, 0)).rgb;

    color = PbrNeutralToneMap(color * post.exposure);

    if (post.encodeSrgb != 0) {
        color = LinearToSrgb(saturate(color));
    }
    return float4(ApplyOutlines(color, pixel), 1.0);
}
