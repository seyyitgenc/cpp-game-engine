// Geometry pass: writes every visible mesh's surface attributes into the
// G-buffer (common/gbuffer.slang), unlit.

import common.frame;
import common.gbuffer;

// Per draw. The C++ half is GeometryPushConstants (Renderer/ShaderTypes.hpp).
// 112 bytes, under the 128 every Vulkan implementation guarantees.
struct DrawData {
    float4x4 model;
    float4 baseColorFactor; // linear RGBA; alpha unused until alpha modes exist
    float4 emissive;        // rgb = emissiveFactor * emissiveStrength, a unused
    float metallicFactor;
    float roughnessFactor;
    uint entityId;
    float _pad;
};

[[vk::push_constant]] ConstantBuffer<DrawData> draw;

// Mesh.hpp: Vertex::GetAttributeDescriptions()
struct VSInput {
    [[vk::location(0)]] float3 position;
    [[vk::location(1)]] float3 normal;
    [[vk::location(2)]] float2 texCoord;
};

struct VSOutput {
    float4 position : SV_Position;
    float3 worldNormal;
    float2 texCoord; // for the material textures, once they exist
};

// Normals transform by the inverse transpose, the cofactor matrix over the
// determinant. Normalizing takes care of the scale, so the determinant only
// contributes its sign (mirrored transforms). Three cross products instead of
// a 3x3 inverse, and correct under non-uniform scale.
float3 TransformNormal(float4x4 model, float3 n) {
    // m[row][col], so these are the columns of the 3x3 part.
    float3 c0 = float3(model[0][0], model[1][0], model[2][0]);
    float3 c1 = float3(model[0][1], model[1][1], model[2][1]);
    float3 c2 = float3(model[0][2], model[1][2], model[2][2]);

    float3 cofactor = n.x * cross(c1, c2) + n.y * cross(c2, c0) + n.z * cross(c0, c1);
    float determinant = dot(c0, cross(c1, c2));
    return determinant < 0.0 ? -cofactor : cofactor;
}

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    float4 worldPosition = mul(draw.model, float4(input.position, 1.0));

    VSOutput output;
    output.position = mul(camera.viewProjection, worldPosition);
    output.worldNormal = TransformNormal(draw.model, input.normal);
    output.texCoord = input.texCoord;
    return output;
}

struct GBufferOutput {
    float4 baseColor : SV_Target0;
    float2 normal    : SV_Target1;
    float4 material  : SV_Target2;
    float4 emissive  : SV_Target3;
    uint entityId    : SV_Target4;
};

[shader("fragment")]
GBufferOutput fragMain(VSOutput input) {
    // Interpolation shortens normals; the octahedral encoding needs unit length.
    float3 N = normalize(input.worldNormal);

    GBufferOutput output;
    output.baseColor = float4(draw.baseColorFactor.rgb, 1.0);
    output.normal = EncodeNormal(N);
    // No occlusion map yet, so nothing is occluded.
    output.material = float4(1.0, draw.roughnessFactor, draw.metallicFactor, 1.0);
    output.emissive = float4(draw.emissive.rgb, 1.0);
    output.entityId = draw.entityId;
    return output;
}
