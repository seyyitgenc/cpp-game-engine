// Post-process pass: exposure, then tonemapping, from linear HDR into the
// scene image the Viewport shows, then the editor's outlines. sRGB encoding
// happens here when the target format does not do it (the scene image is
// UNORM).

import common.frame;
import common.fullscreen;

// Set 1 (PostProcessPass::CreateDescriptorSet): the lighting pass output and
// the G-buffer's entity ids and depth. Set 0 is the per-frame global set; the
// outlines use its camera.
[[vk::binding(0, 1)]] Texture2D<float4> hdrColor;
[[vk::binding(1, 1)]] Texture2D<uint> entityIds;
[[vk::binding(2, 1)]] Texture2D<float> depth;

// C++: PostProcessPushConstants in Renderer/ShaderTypes.hpp.
struct PostData {
    float exposure;
    uint encodeSrgb; // 1 if the target format does not encode by itself
    // Entity ids to outline, [begin, end). Id 0 (nothing drawn) is in neither.
    uint selectedBegin;
    uint selectedEnd;
    uint hoveredBegin;
    uint hoveredEnd;
};

[[vk::push_constant]] ConstantBuffer<PostData> post;

// Applied after encoding, so these are the colors that show.
static const int OUTLINE_RADIUS = 2; // pixels, outside the silhouette
static const float3 SELECTED_OUTLINE = float3(1.0, 0.6, 0.1);
static const float4 HOVERED_OUTLINE = float4(1.0, 1.0, 1.0, 0.5); // a: opacity
// How much nearer than the outlined entity a pixel may be and still take its
// outline, as a share of the distance: enough for a neighbor on the same
// surface.
static const float OUTLINE_DEPTH_TOLERANCE = 0.01;

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

bool IsSelected(uint id) {
    return id >= post.selectedBegin && id < post.selectedEnd;
}

bool IsHovered(uint id) {
    return id >= post.hoveredBegin && id < post.hoveredEnd;
}

// Distance along the view direction, from the depth the camera's projection
// wrote (perspective or orthographic).
float ViewDepth(float d) {
    const float4x4 p = camera.projection;
    return (p[2][3] - d * p[3][3]) / (p[2][2] - d * p[3][2]);
}

float3 ApplyOutlines(float3 color, int2 pixel) {
    // The same for every pixel of the draw, so this costs nothing when
    // nothing is selected or hovered.
    if (post.selectedBegin == post.selectedEnd && post.hoveredBegin == post.hoveredEnd) return color;

    // Only outside the silhouette, so an outline never covers its entities.
    const uint center = entityIds.Load(int3(pixel, 0));
    const bool outsideSelected = !IsSelected(center);
    const bool outsideHovered = !IsHovered(center);
    if (!outsideSelected && !outsideHovered) return color;
    const float centerDepth = ViewDepth(depth.Load(int3(pixel, 0)));

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
            const int3 texel = int3(clamp(pixel + int2(x, y), int2(0), last), 0);
            const uint id = entityIds.Load(texel);
            const bool selected = outsideSelected && IsSelected(id);
            const bool hovered = outsideHovered && IsHovered(id);
            // Only on what lies behind: drawn around something in front of
            // the entity, it would look like that thing's own outline.
            if ((selected || hovered) && centerDepth >= ViewDepth(depth.Load(texel)) * (1.0 - OUTLINE_DEPTH_TOLERANCE)) {
                nearSelected = nearSelected || selected;
                nearHovered = nearHovered || hovered;
            }
        }
    }

    if (nearSelected) return SELECTED_OUTLINE;
    if (nearHovered) return lerp(color, HOVERED_OUTLINE.rgb, HOVERED_OUTLINE.a);
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
