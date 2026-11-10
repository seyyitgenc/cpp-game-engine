// Sprite pass (Renderer/SpritePass): textured quads, unlit, into the sprite
// layer the post-process pass lays over the tonemapped scene. Colors go out
// as the scene image holds them: sRGB encoded, here premultiplied by alpha.
// Each pixel a sprite shows on also takes its entity id and draw order.

import common.color;
import common.frame;

// Renderer/TextureTable.hpp: Capacity
static const uint MAX_TEXTURES = 1024;

// Renderer/ShaderTypes.hpp: GpuSprite
struct GpuSprite {
    float4x4 model; // the unit square (0..1 in x and y) to world space
    float4 color;   // linear RGB tint, a: opacity
    float4 uvRect;  // xy: the UV at the square's (0, 0) corner, zw: the step to its (1, 1)
    uint entityId;
    // Scalars, not a uint3: std430 would align that to 16 bytes.
    uint _pad0;
    uint _pad1;
    uint _pad2;
};

// Per batch. The C++ half is SpritePushConstants (Renderer/ShaderTypes.hpp).
// The texture is the same for a whole draw, so indexing the table with it
// needs no non-uniform indexing feature.
struct BatchData {
    uint texture;
};

[[vk::push_constant]] ConstantBuffer<BatchData> batch;

// Set 1: Renderer/TextureTable. Set 2: this frame's sprites, in draw order.
[[vk::binding(0, 1)]] Sampler2D textures[MAX_TEXTURES];
[[vk::binding(0, 2)]] StructuredBuffer<GpuSprite> sprites;

// Below what an 8-bit target can show. Such pixels are not picked either.
static const float MIN_ALPHA = 1.0 / 255.0;

// Two triangles a sprite, no vertex buffer (SpritePass: VerticesPerSprite).
static const float2 CORNERS[6] = {
    float2(0.0, 0.0), float2(1.0, 0.0), float2(1.0, 1.0),
    float2(0.0, 0.0), float2(1.0, 1.0), float2(0.0, 1.0)
};

struct VSOutput {
    float4 position : SV_Position;
    float2 texCoord;
    nointerpolation float4 color;
    nointerpolation uint entityId;
    nointerpolation uint order;
};

[shader("vertex")]
VSOutput vertMain(uint vertexId : SV_VulkanVertexID) {
    const uint index = vertexId / 6;
    const GpuSprite sprite = sprites[index];
    const float2 corner = CORNERS[vertexId % 6];

    VSOutput output;
    output.position = mul(camera.viewProjection, mul(sprite.model, float4(corner, 0.0, 1.0)));
    output.texCoord = sprite.uvRect.xy + corner * sprite.uvRect.zw;
    output.color = sprite.color;
    output.entityId = sprite.entityId;
    output.order = index + 1;
    return output;
}

struct SpriteOutput {
    float4 color  : SV_Target0;
    uint entityId : SV_Target1;
    // 1 + the sprite's place in the draw order: the pixel's topmost sprite,
    // which is what tells the outlines which of two sprites is in front.
    uint order    : SV_Target2;
};

[shader("fragment")]
SpriteOutput fragMain(VSOutput input) {
    // Sampled before the discard, which would leave the derivatives, and so
    // the mip selection, undefined.
    const float4 color = saturate(textures[batch.texture].Sample(input.texCoord) * input.color);
    if (color.a < MIN_ALPHA) discard;

    SpriteOutput output;
    output.color = float4(LinearToSrgb(color.rgb) * color.a, color.a);
    output.entityId = input.entityId;
    output.order = input.order;
    return output;
}
