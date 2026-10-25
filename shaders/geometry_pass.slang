// Geometry pass: writes every visible mesh's surface attributes into the
// G-buffer (common/gbuffer.slang), unlit.

import common.frame;
import common.gbuffer;

// Renderer/TextureTable.hpp: Capacity
static const uint MAX_TEXTURES = 1024;

// Per draw. The C++ half is GeometryPushConstants (Renderer/ShaderTypes.hpp).
struct DrawData {
    float4x4 model;
    uint materialIndex;
    uint entityId;
    uint2 _pad;
};

[[vk::push_constant]] ConstantBuffer<DrawData> draw;

// Renderer/ShaderTypes.hpp: GpuMaterial. Textures are slots in `textures`.
struct GpuMaterial {
    float4 baseColorFactor; // linear RGBA
    float3 emissive;        // emissiveFactor * emissiveStrength
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    float occlusionStrength;
    float alphaCutoff;
    uint baseColorTexture;
    uint metallicRoughnessTexture;
    uint normalTexture;
    uint occlusionTexture;
    uint emissiveTexture;
    // Scalars, not a uint3: std430 would align that to 16 bytes.
    uint _pad0;
    uint _pad1;
    uint _pad2;
};

// Set 1: Renderer/TextureTable. Set 2: this frame's materials (GeometryPass).
[[vk::binding(0, 1)]] Sampler2D textures[MAX_TEXTURES];
[[vk::binding(0, 2)]] StructuredBuffer<GpuMaterial> materials;

// The alpha-masked pipeline only: discard in the opaque one would cost it its
// early depth test.
[[vk::constant_id(0)]] const bool ALPHA_MASK = false;

// Mesh.hpp: Vertex::GetAttributeDescriptions()
struct VSInput {
    [[vk::location(0)]] float3 position;
    [[vk::location(1)]] float3 normal;
    [[vk::location(2)]] float2 texCoord;
    [[vk::location(3)]] float4 tangent; // w: handedness
};

struct VSOutput {
    float4 position : SV_Position;
    float3 worldNormal;
    float4 worldTangent; // w: handedness, flipped for a mirroring transform
    float2 texCoord;
};

// Normals transform by the inverse transpose, the cofactor matrix over the
// determinant. Normalizing takes care of the scale, so the determinant only
// contributes its sign (mirrored transforms), which the caller applies. Three
// cross products instead of a 3x3 inverse, and correct under non-uniform
// scale.
float3 CofactorTimes(float4x4 model, float3 n) {
    // m[row][col], so these are the columns of the 3x3 part.
    float3 c0 = float3(model[0][0], model[1][0], model[2][0]);
    float3 c1 = float3(model[0][1], model[1][1], model[2][1]);
    float3 c2 = float3(model[0][2], model[1][2], model[2][2]);

    return n.x * cross(c1, c2) + n.y * cross(c2, c0) + n.z * cross(c0, c1);
}

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    float4 worldPosition = mul(draw.model, float4(input.position, 1.0));
    float3x3 basis = (float3x3)draw.model;
    float mirror = determinant(basis) < 0.0 ? -1.0 : 1.0;

    VSOutput output;
    output.position = mul(camera.viewProjection, worldPosition);
    output.worldNormal = CofactorTimes(draw.model, input.normal) * mirror;
    // Tangents lie in the surface, so they transform like positions. A
    // mirroring transform reverses the bitangent cross(normal, tangent) gives.
    output.worldTangent = float4(mul(basis, input.tangent.xyz), input.tangent.w * mirror);
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
GBufferOutput fragMain(VSOutput input, bool frontFace : SV_IsFrontFace) {
    GpuMaterial material = materials[draw.materialIndex];
    float2 uv = input.texCoord;

    // Every map is sampled before a discard, which would leave the quad's
    // derivatives, and so the mip selection, undefined.
    float4 baseColor = material.baseColorFactor * textures[material.baseColorTexture].Sample(uv);
    float4 metallicRoughness = textures[material.metallicRoughnessTexture].Sample(uv);
    float3 tangentNormal = textures[material.normalTexture].Sample(uv).xyz * 2.0 - 1.0;
    float occlusionSample = textures[material.occlusionTexture].Sample(uv).r;
    float3 emissiveSample = textures[material.emissiveTexture].Sample(uv).rgb;

    if (ALPHA_MASK && baseColor.a < material.alphaCutoff) discard;

    // Interpolation shortens and tilts the basis: renormalize, and make the
    // tangent perpendicular to the normal again.
    float3 N = normalize(input.worldNormal);
    float3 T = input.worldTangent.xyz - N * dot(N, input.worldTangent.xyz);
    T = dot(T, T) > 1e-12 ? normalize(T) : float3(0.0);
    float3 B = cross(N, T) * input.worldTangent.w;
    // Only double-sided materials draw back faces; light them from their side.
    if (!frontFace) {
        N = -N;
        T = -T;
        B = -B;
    }
    tangentNormal.xy *= material.normalScale;
    N = normalize(T * tangentNormal.x + B * tangentNormal.y + N * tangentNormal.z);

    GBufferOutput output;
    output.baseColor = float4(baseColor.rgb, 1.0);
    // The octahedral encoding needs unit length.
    output.normal = EncodeNormal(N);
    output.material = float4(1.0 + material.occlusionStrength * (occlusionSample - 1.0),
                             material.roughnessFactor * metallicRoughness.g,
                             material.metallicFactor * metallicRoughness.b,
                             1.0);
    output.emissive = float4(material.emissive * emissiveSample, 1.0);
    output.entityId = draw.entityId;
    return output;
}
