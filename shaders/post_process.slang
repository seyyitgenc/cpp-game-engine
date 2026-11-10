// Post-process pass: exposure, then tonemapping, from linear HDR into the
// scene image the Viewport shows, then the editor's grid, the sprite layer
// and the editor's outlines. sRGB encoding happens here when the target
// format does not do it (the scene image is UNORM).

import common.color;
import common.frame;
import common.fullscreen;

// Set 1 (PostProcessPass::CreateDescriptorSet): the lighting pass output, the
// G-buffer's entity ids and depth, and the sprite pass outputs. Set 0 is the
// per-frame global set; the grid and the outlines use its camera.
[[vk::binding(0, 1)]] Texture2D<float4> hdrColor;
[[vk::binding(1, 1)]] Texture2D<uint> entityIds;
[[vk::binding(2, 1)]] Texture2D<float> depth;
// sRGB encoded and premultiplied by alpha (shaders/sprite.slang).
[[vk::binding(3, 1)]] Texture2D<float4> spriteLayer;
// 1 + the draw order of the pixel's topmost sprite; 0 where there is none.
[[vk::binding(4, 1)]] Texture2D<uint> spriteOrder;

// C++: PostProcessPushConstants in Renderer/ShaderTypes.hpp.
struct PostData {
    float exposure;
    uint encodeSrgb; // 1 if the target format does not encode by itself
    // Entity ids to outline, [begin, end). Id 0 (nothing drawn) is in neither.
    uint selectedBegin;
    uint selectedEnd;
    uint hoveredBegin;
    uint hoveredEnd;
    uint gridPlane; // GRID_*
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

// Renderer/ShaderTypes.hpp: GridPlane
static const uint GRID_NONE = 0;
static const uint GRID_XZ = 1; // y = 0, for 3D scenes
static const uint GRID_XY = 2; // z = 0, for 2D scenes

// Applied after encoding, as the outlines are. Meters: a line every cell, a
// stronger one every GRID_MAJOR cells.
static const float GRID_CELL = 1.0;
static const float GRID_MAJOR = 10.0;
static const float3 GRID_COLOR = float3(0.6, 0.6, 0.6);
static const float GRID_MINOR_OPACITY = 0.3;
static const float GRID_MAJOR_OPACITY = 0.55;
static const float GRID_AXIS_OPACITY = 0.9;
// Blender's axis colors, as the Viewport's orientation gizmo has them.
static const float3 GRID_AXIS_X = float3(1.0, 0.2, 0.32);
static const float3 GRID_AXIS_Y = float3(0.545, 0.863, 0.0);
static const float3 GRID_AXIS_Z = float3(0.157, 0.565, 1.0);
// Fades out between these distances from the camera, as multiples of its
// height above the plane, at least GRID_MIN_FADE meters: far lines would
// shimmer, and a camera high up sees farther.
static const float GRID_FADE_START = 15.0;
static const float GRID_FADE_END = 40.0;
static const float GRID_MIN_FADE = 20.0;
// How much farther than the scene's depth the grid may be and still show,
// as a share of the distance, so a surface lying on the plane does not
// fight it.
static const float GRID_DEPTH_TOLERANCE = 0.001;

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
    const uint centerOrder = spriteOrder.Load(int3(pixel, 0));

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
            if (!selected && !hovered) continue;
            // Only on what lies behind: drawn around something in front of
            // the entity, it would look like that thing's own outline.
            // Sprites write no depth, so a sprite is in front of another
            // when it was drawn later.
            const uint order = spriteOrder.Load(texel);
            if (order != 0 && centerOrder > order) continue;
            if (centerDepth >= ViewDepth(depth.Load(texel)) * (1.0 - OUTLINE_DEPTH_TOLERANCE)) {
                nearSelected = nearSelected || selected;
                nearHovered = nearHovered || hovered;
            }
        }
    }

    if (nearSelected) return SELECTED_OUTLINE;
    if (nearHovered) return lerp(color, HOVERED_OUTLINE.rgb, HOVERED_OUTLINE.a);
    return color;
}

// Coverage of the lines of a grid of square cells at coord, about a pixel
// wide and antialiased. It fades out as the cells shrink towards a few
// pixels, before they would shimmer, which leaves the next level's lines.
float GridLines(float2 coord, float cell) {
    const float2 scaled = coord / cell;
    const float2 cellsPerPixel = max(fwidth(scaled), 1e-6);
    const float2 pixelsToLine = abs(frac(scaled - 0.5) - 0.5) / cellsPerPixel;
    const float line = 1.0 - saturate(min(pixelsToLine.x, pixelsToLine.y));
    return line * (1.0 - smoothstep(0.1, 0.3, max(cellsPerPixel.x, cellsPerPixel.y)));
}

// Coverage of the line where coord is 0, a little wider than the others.
float AxisLine(float coord) {
    return 1.0 - saturate(abs(coord) / max(fwidth(coord), 1e-6) * 0.6);
}

// Where the pixel's view ray meets the plane. Every derivative is taken
// before anything that differs per pixel decides to skip work, as fwidth()
// needs.
float3 ApplyGrid(float3 color, float2 pixelCenter) {
    // The same for every pixel of the draw.
    if (post.gridPlane == GRID_NONE) return color;

    // The ray from the near plane to the far one: parallel rays for an
    // orthographic camera, from the eye for a perspective one.
    uint width, height;
    depth.GetDimensions(width, height);
    const float2 ndc = pixelCenter / float2(width, height) * 2.0 - 1.0;
    const float4 nearPoint = mul(camera.inverseViewProjection, float4(ndc, 0.0, 1.0));
    const float4 farPoint = mul(camera.inverseViewProjection, float4(ndc, 1.0, 1.0));
    const float3 origin = nearPoint.xyz / nearPoint.w;
    const float3 ray = farPoint.xyz / farPoint.w - origin;

    // The coordinate along the plane's normal, and the two in it.
    const bool xy = post.gridPlane == GRID_XY;
    const float originHeight = xy ? origin.z : origin.y;
    const float rayHeight = xy ? ray.z : ray.y;
    const float t = -originHeight / (abs(rayHeight) > 1e-7 ? rayHeight : 1e-7);
    const float3 hit = origin + ray * t;
    const float2 coord = xy ? hit.xy : hit.xz;

    const float minor = GridLines(coord, GRID_CELL);
    const float major = GridLines(coord, GRID_CELL * GRID_MAJOR);
    // coord.y == 0 is the X axis; coord.x == 0 is Z, or Y in a 2D scene.
    const float axisX = AxisLine(coord.y);
    const float axisOther = AxisLine(coord.x);

    // In front of the camera, and not behind what the scene drew there (the
    // background is at depth 1, the far plane).
    const float4 clip = mul(camera.viewProjection, float4(hit, 1.0));
    const float sceneDepth = depth.Load(int3(int2(pixelCenter), 0));
    if (t <= 0.0 || ViewDepth(clip.z / clip.w) > ViewDepth(sceneDepth) * (1.0 + GRID_DEPTH_TOLERANCE)) return color;

    const float cameraHeight = abs(xy ? camera.position.z : camera.position.y);
    const float fadeEnd = max(cameraHeight * GRID_FADE_END, GRID_MIN_FADE);
    const float fade = 1.0 - smoothstep(fadeEnd * (GRID_FADE_START / GRID_FADE_END), fadeEnd, length(hit - camera.position.xyz));

    color = lerp(color, GRID_COLOR, max(minor * GRID_MINOR_OPACITY, major * GRID_MAJOR_OPACITY) * fade);
    color = lerp(color, xy ? GRID_AXIS_Y : GRID_AXIS_Z, axisOther * GRID_AXIS_OPACITY * fade);
    return lerp(color, GRID_AXIS_X, axisX * GRID_AXIS_OPACITY * fade);
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
    color = ApplyGrid(color, input.position.xy);

    // Over the grid: sprites write no depth for it to hide behind.
    const float4 sprites = spriteLayer.Load(int3(pixel, 0));
    color = color * (1.0 - sprites.a) + sprites.rgb;

    return float4(ApplyOutlines(color, pixel), 1.0);
}
